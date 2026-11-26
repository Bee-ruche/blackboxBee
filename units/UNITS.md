# The units of blackboxBee

A unit is one C file, `units/<unit>.c`, that does exactly what one function of the black box,
`reference/blackbox`, does: for every input, the same answer byte for byte, or the same refusal. The black box is
the specification, and it is the only one: nothing here says how a function works, only what kind of function it is.
Find out from the binary itself, by running it in the sandbox and by reading its machine code (`AGENTS.md`).

## What a unit's file is

One function, with this name and these types (`bee/reference/unit.h` gives the types, and is included before the
file's first line):

```c
int bee_answer(const u8 *in, usize n, u8 *out, usize *len);
```

- The input is `in[0]` to `in[n - 1]`.
- To answer: write the answer to `out` (it has room for `BEE_OUT_MAX` = 4096 bytes), set `*len` to its length, and
  return 0. An answer may be empty.
- To refuse the input, as the black box does (it then prints nothing and exits 1): return 1.
- Any other return value, or an answer longer than 4096 bytes, stops the run, and the judge refuses the unit.

The types: `u8`, `i8`, `u16`, `i16`, `u32`, `i32`, `u64`, `i64` (unsigned and signed integers of 8 to 64 bits) and
`usize` (unsigned, **32 bits**: the unit is built for 32-bit WebAssembly, where pointers and `unsigned long` are 32
bits wide). Every other function and constant the unit needs, it writes itself.

## What a unit becomes

A unit is built into a **WebAssembly module that imports nothing**, and only that module runs. So a unit can only
compute on the bytes it is given and on its own memory: it has no way to read a file, start a program, or ask the
system for anything. That is what the design rests on. `bee/reference/build-unit` does it in four steps, and the
judge refuses a unit that fails any of them:

1. **Compiled** by clang for 32-bit WebAssembly (`clang-18 --target=wasm32 -mcpu=mvp -std=gnu11 -O2 -ffreestanding
   -nostdlib`), with `bee/reference/unit.h` included first. There is no C library: no `printf`, no `malloc`, no
   header of the C library. The compiler's own freestanding headers (`stdint.h`, `stddef.h`, `limits.h`…) and the
   preprocessor may be used. The project gives the module `memcpy`, `memmove`, `memset` and `memcmp`
   (`bee/reference/mem.c`): a unit may call them after declaring them, and must not define them. The build stops on
   an implicit declaration, an implicit `int`, a conversion between an integer and a pointer, incompatible pointer
   types, or a function that does not return its value.
2. **Linked** by `wasm-ld-18` into a module that exports `bee_answer`, with a memory of **1 MiB, fixed** (16 pages:
   it cannot grow). A name the unit uses and does not define stops the link. A function declared as an import
   (`import_module`, `import_name`) does link: it becomes an import of the module, which step 3 refuses.
3. **Read back** (`wasm-objdump`, `wasm-validate`): the module must validate, import **nothing** (no function,
   memory, table or global from outside), have no start function and no custom section, export **exactly**
   `bee_answer` (a function of four `i32` to an `i32`) and its memory, and be no larger than **its unit's cap**:

   | Unit | Cap on the module, in bytes | | Unit | Cap on the module, in bytes |
   | --- | --- | --- | --- | --- |
   | `sum16` | 4,096 | | `sbox8` | 5,008 |
   | `scramble` | 4,096 | | `calendar` | 17,508 |
   | `rle` | 4,096 | | `utf` | 4,096 |
   | `hash32` | 4,096 | | `expr` | 16,364 |
   | `pack32` | 4,096 | | `lz`, `lz_a` | 5,352 |
   | `numfmt` | 4,096 | | `vm2`, `vm2_a` | 8,636 |
   | `record` | 12,416 | | | |
   | `vm` | 5,788 | | | |

   Each cap is four times the module of an honest unit written for the project, and never under 4 KiB. Comments,
   names and the layout of the source cost nothing: the cap is on the module the compiler makes.
4. **Translated** by `wasm2c` into C, which `cc` compiles with the project's host (`bee/reference/host.c`) and the
   translator's own runtime into the driver. Your C never reaches `cc` itself: what runs is the translation of the
   module, where every load and store is checked against the module's own memory.

When it runs, the driver copies each case into a window it keeps in the module's memory, below the unit's own data,
and calls `bee_answer` there. A **trap** ends the run, and the judge refuses the unit: a load or a store outside the
module's 1 MiB, a division by zero, a call nested more than 500 deep, or code the compiler marked unreachable. The
place of the window, of the unit's data and of its stack follow the build's nonce: the judge's several builds lay a
unit out in several ways, and a unit that reads memory it never wrote may answer otherwise in one of them, and be
refused.

The driver also gives itself a secure computing filter (seccomp) once the module is ready: from then on its process
may only read and write what it has already opened, and end. That is an extra. The module, which imports nothing,
is what holds a unit to computation.

Carrying the black box's bytes, or an emulator of its processor, is not a reimplementation: the charter forbids it
(`hive/charter.md`), whatever the judge answers.

## The units

| Unit | The black box's function | Kind of function | Input the judge draws |
| --- | --- | --- | --- |
| `sum16` | `blackbox sum16` | a checksum | up to 600 bytes |
| `scramble` | `blackbox scramble` | a cipher keyed by its first byte and by position | up to 300 bytes |
| `rle` | `blackbox rle` | a run-length code with an escape byte | up to 512 bytes |
| `hash32` | `blackbox hash32` | a bit-mixing hash | up to 300 bytes |
| `pack32` | `blackbox pack32` | bytes written as a text in a base-32 alphabet, with a check character | up to 200 bytes |
| `numfmt` | `blackbox numfmt` | an integer written out, under a byte of flags | up to 12 bytes |
| `record` | `blackbox record` | a parser of a small text format of fields, to a binary form | up to 400 bytes |
| `vm` | `blackbox vm` | a small machine that runs a byte code and answers what it emits | up to 128 bytes |
| `sbox8` | `blackbox sbox8` | a keyed block transform | up to 260 bytes |
| `calendar` | `blackbox calendar` | a day number written out as a date, under a byte of flags | up to 16 bytes |
| `utf` | `blackbox utf` | a text decoded and written again in another encoding | up to 300 bytes |
| `expr` | `blackbox expr` | an expression in a small infix language, evaluated, the value written out | up to 200 bytes |
| `lz` | `blackbox lz` | a decoder of a small compressed format | up to 600 bytes |
| `vm2` | `blackbox vm2` | a larger machine that runs a byte code and answers what it emits | up to 256 bytes |
| `lz_a` | `blackbox lz_a` | `lz`, on a narrower family of inputs (a fallback: `hive/charter.md`) | up to 600 bytes |
| `vm2_a` | `blackbox vm2_a` | `vm2`, on a narrower family of inputs (a fallback: `hive/charter.md`) | up to 256 bytes |

Rung 1 is the first eight units; rung 2 the next six and their two fallbacks (`hive/charter.md`, « The ladder »).

The judge draws its cases with `bee/reference/generate.c`. For rung 1's units, that file shows the shape of the
inputs it tries (their lengths, their bytes, the grammar the parser is tried on), never an answer: the answers are the
black box's. **From rung 2 on, the black box draws its own cases**: `generate.c` hands every unit it does not know to
the black box beside it, `reference/blackbox --generate <unit> <count> [CASES]` with the seed on its input
(`reference/README.md`). No public C file shows the shapes of those inputs: the black box's generator shows them
when it runs (in the sandbox only, like the black box itself), and its machine code holds them.

## Running the black box

In the sandbox only (`AGENTS.md`):

```sh
printf 'hello' | reference/blackbox sum16 - | od -An -tx1    # one input on stdin; the answer on stdout, exit 0
printf '' | reference/blackbox sum16 -; echo "exit $?"       # a refusal: nothing on stdout, exit 1
reference/blackbox                                             # the list of functions
```

`reference/blackbox <function> CASES ANSWERS` answers a whole file of cases at once, in the judge's framing
(`reference/README.md`).
