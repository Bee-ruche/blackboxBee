/* A unit's driver (docs/reference-judge.md of Bee-ruche/hive): reads a file of cases, calls the unit on each, writes
 * one answer per case, in order. The reference's driver is the reference itself: reference/blackbox reads and writes
 * the same files in the same framing.
 *
 *   driver [CASES [ANSWERS]]     by default /cases/cases and /answers/answers, where the hive's judge wants them
 *
 * Cases: <u32 length><bytes>...; answers: <u32 index><u8 status><u32 length><bytes>..., little-endian; a refusal
 * (status 1) carries no bytes.
 *
 * The unit is a WebAssembly module with no imports (bee/reference/build-unit made it, checked it, and had wasm2c
 * translate it to C, compiled here as module.c): it can only compute on its own memory. This host is the only code
 * around it. It instantiates the module, then, for each case, copies the bytes into the window it owns in the
 * module's memory (BEE_WINDOW, below the unit's data: build-unit lays the unit there), calls the export bee_answer,
 * and reads the answer back from the window. A trap (a load or store out of the module's memory, a call too deep, a
 * division by zero, unreachable code) ends the run: the unit answered nothing.
 *
 * An extra, which the design does not rest on: once the module's memory is allocated and the files are open, the host
 * gives itself a secure computing filter (seccomp) where the process may only read and write the descriptors it holds,
 * and end. wasm2c's runtime is compiled without its signal handler (WASM_RT_USE_MMAP=0 and
 * WASM_RT_MEMCHECK_BOUNDS_CHECK=1: explicit bounds checks, plain setjmp and longjmp for traps, memory from calloc at
 * instantiation), so nothing after that point needs another system call.
 */
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "module.h"
#include "wasm-rt-exceptions.h"
#include "wasm-rt-impl.h"

#if defined(__x86_64__)
#define ARCHITECTURE AUDIT_ARCH_X86_64
#elif defined(__aarch64__)
#define ARCHITECTURE AUDIT_ARCH_AARCH64
#else
#error "the driver knows the system calls of x86-64 and ARM64 only"
#endif

#ifndef BEE_WINDOW
#error "build-unit gives the window's address (BEE_WINDOW)"
#endif

/* The window, in the module's memory (units/UNITS.md): the case, the room for the answer, the answer's length. */
enum { IN_MAX = 65536, OUT_MAX = 4096 };
#define WINDOW_IN ((u32)(BEE_WINDOW))
#define WINDOW_OUT (WINDOW_IN + IN_MAX)
#define WINDOW_LEN (WINDOW_OUT + OUT_MAX)
#define WINDOW_END (WINDOW_LEN + 4)

#define ALLOW(call) BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (call), 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

static int walled;

static void leave(int status)
{
    if (walled)  /* exit() would run what the C library does at exit; the system call alone is enough */
        syscall(SYS_exit_group, status);
    exit(status);
}

static void fail(const char *what)
{
    fprintf(stderr, "driver: %s\n", what);
    leave(3);
}

/* Read and write, end, and return from a signal handler; nothing else. x32 calls (bit 30) are another table: refused. */
static void wall(void)
{
    static struct sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ARCHITECTURE, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, 0x40000000, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
        ALLOW(SYS_read),
        ALLOW(SYS_write),
        ALLOW(SYS_exit),
        ALLOW(SYS_exit_group),
        ALLOW(SYS_rt_sigreturn),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
    };
    struct sock_fprog program = {sizeof rules / sizeof rules[0], rules};
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0 || prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) != 0)
        fail("cannot give itself its secure computing filter");
    walled = 1;
}

static void put32(FILE *f, unsigned long v)
{
    unsigned char b[4];
    b[0] = (unsigned char)v;
    b[1] = (unsigned char)(v >> 8);
    b[2] = (unsigned char)(v >> 16);
    b[3] = (unsigned char)(v >> 24);
    if (fwrite(b, 1, 4, f) != 4)
        fail("cannot write the answers");
}

static void store32(u8 *at, u32 v)
{
    at[0] = (u8)v;
    at[1] = (u8)(v >> 8);
    at[2] = (u8)(v >> 16);
    at[3] = (u8)(v >> 24);
}

static u32 load32(const u8 *at)
{
    return (u32)at[0] | (u32)at[1] << 8 | (u32)at[2] << 16 | (u32)at[3] << 24;
}

/* One call of the unit, inside the runtime's trap guard: its status, or the trap's code through *trap. */
static u32 call(w2c_unit *unit, u32 n, wasm_rt_trap_t *trap)
{
    wasm_rt_trap_t code = (wasm_rt_trap_t)wasm_rt_impl_try();
    if (code != WASM_RT_TRAP_NONE) {
        *trap = code;
        return 0;
    }
    return w2c_unit_bee_answer(unit, WINDOW_IN, n, WINDOW_OUT, WINDOW_LEN);
}

int main(int argc, char **argv)
{
    static char buffer[1 << 16];
    static w2c_unit unit;
    unsigned char *cases = NULL, *grown;
    size_t have = 0, room = 0, got, at = 0, index = 0;
    u32 n, len, status;
    wasm_rt_memory_t *memory;
    wasm_rt_trap_t trap;
    FILE *in, *answers;
    if (argc > 3)
        fail("usage: driver [CASES [ANSWERS]]");
    in = fopen(argc > 1 ? argv[1] : "/cases/cases", "rb");
    if (!in)
        fail("cannot open the cases");
    for (;;) {
        if (have == room) {
            room = room ? room * 2 : 1 << 16;
            if (room > (1ul << 30) || !(grown = realloc(cases, room)))
                fail("the cases are too large");
            cases = grown;
        }
        got = fread(cases + have, 1, room - have, in);
        have += got;
        if (got == 0)
            break;
    }
    if (ferror(in))
        fail("cannot read the cases");
    fclose(in);
    answers = fopen(argc > 2 ? argv[2] : "/answers/answers", "wb");
    if (!answers || setvbuf(answers, buffer, _IOFBF, sizeof buffer) != 0)
        fail("cannot open the answers");
    wasm_rt_init();
    wasm2c_unit_instantiate(&unit);  /* the module with no imports: an instance alone, nothing given to it */
    memory = w2c_unit_memory(&unit);
    if (!memory->data || memory->size < WINDOW_END)
        fail("the unit's memory does not hold the window");
    wall();
    while (at < have) {
        if (have - at < 4)
            fail("the cases are not well framed");
        n = load32(cases + at);
        at += 4;
        if (have - at < n)
            fail("the cases are not well framed");
        if (n > IN_MAX)
            fail("a case is longer than the window");
        memcpy(memory->data + WINDOW_IN, cases + at, n);
        store32(memory->data + WINDOW_LEN, 0);
        trap = WASM_RT_TRAP_NONE;
        status = call(&unit, n, &trap);
        if (trap != WASM_RT_TRAP_NONE) {
            fprintf(stderr, "driver: the unit trapped (%s)\n", wasm_rt_strerror(trap));
            leave(3);
        }
        len = load32(memory->data + WINDOW_LEN);
        if (status > 1 || (status == 0 && len > OUT_MAX))
            fail("the unit returned neither an answer nor a refusal");
        put32(answers, index++);
        if (fputc((int)status, answers) == EOF)
            fail("cannot write the answers");
        put32(answers, status == 0 ? len : 0);
        if (status == 0 && fwrite(memory->data + WINDOW_OUT, 1, len, answers) != len)
            fail("cannot write the answers");
        at += n;
    }
    /* The block of cases is not freed, the instance not torn down and the file not closed: each could ask the kernel
       for what the filter refuses (munmap, close). The process ends here, its answers written. */
    if (fflush(answers) != 0)
        fail("cannot write the answers");
    leave(0);
    return 0;
}
