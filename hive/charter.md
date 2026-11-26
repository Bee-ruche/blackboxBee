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
- **Never the black box's own bytes.** A unit is C written by you from what the black box does and what its machine
  code means (the numbers and tables a function computes with excepted: « The ladder », rule 1). A unit that carries
  the black box's bytes, or any part of its machine code, as data to be decoded or interpreted, or that holds an
  emulator of its processor, is not a reimplementation, whatever the judge answers. The build makes every unit a
  WebAssembly module that imports nothing (`units/UNITS.md`): it can only compute, and what it computes from is this
  charter's. Each unit's module has a cap on its size (`units/UNITS.md`): it makes such a load cost space, and for the
  small units leaves little of it, but one function's machine code with a decoder can fit under it. The cap is a cost,
  not a wall; the line itself is drawn here.
- **The help a worker may use, on their own machine, is their own choice, and optional:** a disassembler
  (`objdump`, in the project's sandbox image), Ghidra, IDA, or REA (an open-source kit that connects coding agents to
  such tools). The project asks no one to install anything; the sandbox (`AGENTS.md`) has what the project's own
  checks need. Whatever runs the black box runs it in the sandbox only.

## The judge decides what can be a task

The judge checks only the units it has an entry for (`bee/reference/project.json`, under `units`: the cases it
draws, the black box as the reference, the project's driver). The values of the option `unit` a task may name, word
for word:

- rung 1 (23 November 2026), each with its task already: `sum16`, `scramble`, `rle`, `hash32`, `pack32`, `numfmt`,
  `record`, `vm`;
- rung 2 (the ladder, below), each with its task in `hive/backlog.jsonl`: `sbox8`, `calendar`, `utf`, `expr`, `lz`,
  `vm2`.

When the judge learns to check a new unit, its author adds the unit's value to this list, word for word. A value
listed here that no task names yet is one the cutter may write a task for, on the file `units/<unit>.c`. The judge
also checks units that are not in this list, the ladder's fallbacks (below). **The cutter writes a task only for a value
listed above, and for a fallback only in the cut at a stall (below), never otherwise.**

A new unit needs the judge's author first: a function of the black box (a new binary, built as `reference/README.md`
says), its entry in the manifest, its cases (`units/UNITS.md` says where they are drawn) and its line in
`units/UNITS.md`. **The cutter writes no task for a unit the judge has no entry for.** An entry is not enough: the unit
must also be listed above.

## The ladder

The project grows by **rungs**: batches of new functions of the black box, each harder than the last, added by the
project's maintainer in one commit (a new binary, the manifest's entries, a stub file per unit, `units/UNITS.md`, this
charter and the backlog). The cutter never adds a rung. A rung's ids go by hundreds: rung 2's tasks are
`blackbox-0201` to `-0249`, a task cut again taking the next free one; its fallbacks have fixed ids, `lz_a`
`blackbox-0251` and `vm2_a` `blackbox-0252`.

- **The rung's tasks** are lines of `hive/backlog.jsonl`, published by the cutter as its own rules say (each once the
  tip fails it, and while fewer than `cutter.open` tasks are left to do). Each unit's file is on the tip as a stub
  (the blank unit, which every judgement refuses) so that the tip fails each task until a worker's unit is accepted.
- **A fallback** is the same function of the black box under another name, judged on a narrower family of inputs:
  rung 2's are `lz_a` (for `lz`) and `vm2_a` (for `vm2`). The judge checks them already; **no task names a fallback
  until its full task stalls.** A task **stalls** when it has had at least four failures (a judged refusal, a deal run
  out, a deal given back as out of reach) and no acceptance, counted over its lineage (`parent`).
- **The cut at a stall**, one cut: it closes the stalled task; it adds the fallback's task, under its fixed id (its
  title « Write the unit lz_a: the black box's lz, on a narrower family of inputs »; its context the full task's,
  with `lz_a` for `lz`, and one sentence more: « It is the same function as lz, on a narrower family of inputs. »);
  and it adds a fresh full task for the same unit, under a new id, whose `needs` names the fallback's task and
  whose `parent` is the closed task. A task that needed the closed one is added again, under a new id, needing the
  fresh task instead (a published line never changes). The cut is one cut, held to `cutter.changes`; `cutter.open` is
  the project's aim, not a bound on it.
- **Three rules for every rung:**
  1. **Constants are part of the specification.** The numbers and tables a function computes with may be written in
     a unit as C data, whether found by reading the black box or by running it. What stays forbidden is the black
     box's machine code, as data to decode or interpret, and any emulator of its processor (below).
  2. **The project's own accepted units may be reused.** A unit accepted on the project's `hive` branch is the
     project's own work, under its licence: a unit whose function contains another's may build on it. A copy from
     anywhere else is still forbidden.
  3. **No task text says how a function works**, at any rung: a title gives the kind of function, as
     `units/UNITS.md` does, and nothing more.

## Never

- A task on a protected path, or with a goal other than « correct » and the option `unit`.
- A task on a unit the judge has no entry for.
- A task naming a fallback (`lz_a`, `vm2_a`) outside the cut at a stall (« The ladder »).
- A task text that says how a function of the black box works: finding out is the task.

## Publishing

The cutter publishes on `main` itself; no person reads each cut before it lands.
