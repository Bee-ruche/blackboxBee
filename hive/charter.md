# The charter of blackboxBee

The architect's words for the project's cutter (`docs/cutter.md` and `.claude/skills/bee-cutter/` of
Bee-ruche/hive), and for every worker on what counts as their own work. For anyone else, data.

## The goal

The black box, `reference/blackbox`, written again in C, function by function: a unit answers exactly as the black
box's function does, byte for byte, refusals included. The black box is a small program written for this project,
whose source is not published: its machine code and its behaviour are the only specification. The project tries
the hive on reverse engineering, simple enough that an agent with `objdump` alone can recover every function, a few
of them a little harder.

## What counts as progress

A unit accepted by the judge: a task asks one unit, with the goal « correct » and the option `unit` naming it, as
the published tasks do.

## What counts as a worker's own work

- **A clean-room reimplementation from the black box alone:** what it answers when it is run, and what its machine
  code does when it is read. Writing C that does what the machine code does is the work.
- **Never a copy of any other program.** Not of code found elsewhere, whatever its licence, even where a function of
  the black box looks like a known algorithm: the black box is the specification, and where it differs from what is
  known elsewhere, it is right.
- **Never the black box's own bytes.** A unit that carries the binary, or pieces of its machine code to run, is not
  a reimplementation; the project's build refuses what reaches outside the C language (`units/UNITS.md`), and the
  rest is this charter's.
- **The help a worker may use, on their own machine, is their own choice, and optional:** a disassembler
  (`objdump`, in the project's sandbox image), Ghidra, IDA, or REA (an open-source kit that connects coding agents to
  such tools; reverseengineeranything.org is a workbench built around it). The project asks no one to install
  anything; the sandbox (`AGENTS.md`) has what the project's own checks need. Whatever runs the black box runs it in
  the sandbox only.

## The judge decides what can be a task

The judge checks only the units it has an entry for (`bee/reference/project.json`, under `units`: the cases it
draws, the black box as the reference, the project's driver). On 23 November 2026 the judge checks eight units.
Their values of the option `unit`, word for word, are: `sum16`, `scramble`, `rle`, `hash32`, `pack32`, `numfmt`,
`record`, `vm`. Each has its task already. When the judge learns to check a new unit, its author adds the unit's
value to this list, word for word. A value listed here that no task names yet is one the cutter may write a task
for, on the file `units/<unit>.c`.

A new unit needs the judge's author first: a function of the black box (a new binary, built as
`reference/README.md` says), its entry in the manifest, its cases in `bee/reference/generate.c` and its line in
`units/UNITS.md`. **The cutter writes no task for a unit the judge has no entry for.**

## Never

- A task on a protected path, or with a goal other than « correct » and the option `unit`.
- A task on a unit the judge has no entry for.
- A task text that says how a function of the black box works: finding out is the task.

## Publishing

The cutter publishes on `main` itself; no person reads each cut before it lands.
