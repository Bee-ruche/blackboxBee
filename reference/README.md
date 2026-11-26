# The black box

`blackbox` is the project's reference and its only specification: a program for Linux on x86-64, written for the
Bee project, whose source is not published. Each of its functions is « bytes in, bytes out, or a refusal »;
`units/UNITS.md` names them and says what kind of function each is. It grows by rungs (`hive/charter.md`): rung 2
added six functions and two fallbacks (`units/UNITS.md`) to rung 1's eight, rung 3 five more and three fallbacks;
the functions of each rung keep their behaviour in the next.

**Run it in the project's sandbox only** (`AGENTS.md`): it is project code like any other.

## How it is used

```
blackbox                              the names of its functions, on the error output; exit 2
blackbox <function> -                 one input on stdin (at most 1024 bytes): the answer on stdout and exit 0,
                                      or nothing on stdout and exit 1 when the function refuses the input
blackbox <function> [CASES [ANSWERS]] every case of a file, every answer to a file, in the framing of the hive's
                                      reference judge; by default /cases/cases and /answers/answers
blackbox --generate <unit> <count> [CASES]
                                      <count> cases of a unit of rung 2 on, drawn from the seed (64 hexadecimal
                                      digits) on stdin, framed as the judge wants them; by default /cases/cases
```

Exit 3: a file that cannot be read or written, a case longer than 1024 bytes, or cases not well framed. Exit 2: the
usage (an unknown function or unit, a count below the unit's fixed cases or above 100,000, a seed that is not 64
hexadecimal digits).

The framing is the judge's kind's (`docs/reference-judge.md` of Bee-ruche/hive), little-endian: cases
`<u32 length><bytes>`…; answers `<u32 index><u8 status><u32 length><bytes>`…, where the index is the case's
position, the status 0 for an answer and 1 for a refusal, and a refusal carries no bytes. The default paths are
those the judge gives its reference's driver: the black box is that driver itself, and `bee/reference/build-reference`
copies it as it is. For the units of rung 2 on, the black box is also the generator of cases:
`bee/reference/generate.c` hands it every unit it does not know (`units/UNITS.md`).

## How it was built

Once, in the hive's own judge image (Bee-ruche/hive's `tools/judge/Dockerfile.base` at the commit this project's
workflows name: Ubuntu by fingerprint `24737cef17f4…`, the image the judge labels `bee-common-judge:20be30d11aab28f0`),
twice in a row, the two builds compared byte for byte:

| | |
| --- | --- |
| compiler | `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` |
| flags | `-std=gnu11 -O2 -fno-stack-protector -fcf-protection=none -U_FORTIFY_SOURCE -fno-asynchronous-unwind-tables -fno-ident -s -Wl,--build-id=none` |
| size | 92,216 bytes, stripped, dynamically linked to the C library (symbols of `GLIBC_2.2.5` and `GLIBC_2.34`) |
| SHA-256 | `16adaea27e2c750b2b0afbe3eaf7234f1779be9a545abe4f1a8a06ec4c00ba00` (rung 3; rung 2's was `2fe535b0…`, rung 1's `18f082ac…`) |

Optimised and stripped, without the protections that clutter the machine code (stack protector, control-flow
markers, fortified calls): what is left to read is the functions themselves and the small program around them.
