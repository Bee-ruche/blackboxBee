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
`usize` (unsigned, 64 bits). Every other function and constant the unit needs, it writes itself, as `static`.

## What a unit's file may hold

The rule is checked by `bee/reference/build-unit` before anything is linked, and the judge refuses a unit that
breaks it. It is checked on the whole text, comments and strings included:

- **No preprocessor:** no `#` anywhere (so no `#include` and no `#define`), and no `%:`.
- **No line ending in a backslash.**
- **None of these words:** `asm`, `_Pragma`, nor anything with two underscores in a row (`__`).
- **It calls nothing but itself.** The compiled file may refer to no function or variable outside it, except
  `memcpy`, `memmove`, `memset` and `memcmp`, which a compiler may call on its own (a unit may call them too, after
  declaring them).
- **One global name: `bee_answer`.** Every other function and variable is `static`.

So a unit is plain computation on the bytes it is given: it reads no file, starts no program and makes no call to
the system. The driver holds it to that a second time: it calls the unit under a secure computing filter
(seccomp) it gives itself, where the process may only read and write what it has already opened, and end: any other
system call ends the run. It is built with `cc -std=gnu11 -O2` (gcc, from the judge's Ubuntu image), and the build stops on an
implicit declaration, an implicit `int`, a conversion between an integer and a pointer, incompatible pointer types,
or a function that does not return its value.

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

The judge draws its cases with `bee/reference/generate.c`. That file shows the shape of the inputs it tries (their
lengths, their bytes, the grammar the parser is tried on), never an answer: the answers are the black box's.

## Running the black box

In the sandbox only (`AGENTS.md`):

```sh
printf 'hello' | reference/blackbox sum16 - | od -An -tx1    # one input on stdin; the answer on stdout, exit 0
printf '' | reference/blackbox sum16 -; echo "exit $?"       # a refusal: nothing on stdout, exit 1
reference/blackbox                                             # the list of functions
```

`reference/blackbox <function> CASES ANSWERS` answers a whole file of cases at once, in the judge's framing
(`reference/README.md`).
