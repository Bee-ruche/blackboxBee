/* A unit's driver (docs/reference-judge.md of Bee-ruche/hive): reads a file of cases, calls the unit on each, writes
 * one answer per case, in order. The reference's driver is the reference itself: reference/blackbox reads and writes
 * the same files in the same framing.
 *
 *   driver [CASES [ANSWERS]]     by default /cases/cases and /answers/answers, where the hive's judge wants them
 *
 * Cases: <u32 length><bytes>...; answers: <u32 index><u8 status><u32 length><bytes>..., little-endian; a refusal
 * (status 1) carries no bytes.
 *
 * Before the unit is first called, every case is in memory and the file of answers is open with a buffer of its own;
 * then the driver gives itself a secure computing filter (seccomp): from then on the process may only read and write
 * the descriptors it already holds, and end. Any other system call kills it, and the run fails. A unit is plain
 * computation by the project's rule (units/UNITS.md, checked by build-unit); this is the second wall, for whatever
 * would get past the first: no file opened, no program started, no memory mapped or made executable. (The kernel's
 * strict mode says the same, but it cannot be entered under a filter, and Docker gives every container one: this
 * filter is stacked on Docker's.)
 */
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "unit.h"

#if defined(__x86_64__)
#define ARCHITECTURE AUDIT_ARCH_X86_64
#elif defined(__aarch64__)
#define ARCHITECTURE AUDIT_ARCH_AARCH64
#else
#error "the driver knows the system calls of x86-64 and ARM64 only"
#endif

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

int main(int argc, char **argv)
{
    static u8 out[BEE_OUT_MAX];
    static char buffer[1 << 16];
    u8 *cases = NULL, *grown;
    usize have = 0, room = 0, got, at = 0, n, len, index = 0;
    int status;
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
    wall();
    while (at < have) {
        if (have - at < 4)
            fail("the cases are not well framed");
        n = (usize)cases[at] | (usize)cases[at + 1] << 8 | (usize)cases[at + 2] << 16 | (usize)cases[at + 3] << 24;
        at += 4;
        if (have - at < n)
            fail("the cases are not well framed");
        len = 0;
        status = bee_answer(cases + at, n, out, &len);
        if ((status != 0 && status != 1) || (status == 0 && len > BEE_OUT_MAX))
            fail("the unit returned neither an answer nor a refusal");
        put32(answers, index++);
        if (fputc(status, answers) == EOF)
            fail("cannot write the answers");
        put32(answers, status == 0 ? len : 0);
        if (status == 0 && fwrite(out, 1, len, answers) != len)
            fail("cannot write the answers");
        at += n;
    }
    /* The block of cases is not freed and the file is not closed: either could ask the kernel for what the filter
       refuses (munmap, close). The process ends here, its answers written. */
    if (fflush(answers) != 0)
        fail("cannot write the answers");
    leave(0);
    return 0;
}
