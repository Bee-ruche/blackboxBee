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
  `vm2`;
- rung 3, each with its task in `hive/backlog.jsonl` (some waiting, by `needs`, for a unit of rung 1): `unrle`,
  `unpack32`, `kv`, `alloc`, `frame`;
- rung 4, each with its task in `hive/backlog.jsonl` (vm2sum's waiting, by `needs`, for vm2's): `vm2sum`,
  `tlvcheck`, `fold`;
- rung 5, the last planned, each with its task in `hive/backlog.jsonl` (`parse`'s waiting, by `needs`, for `lex`'s,
  `run`'s for `parse`'s): `lex`, `parse`, `run`.

The judge also checks `match` (rung 4) and its fallback `match_a`, which are **staged**: no task names them, and the
cutter writes none, until the project's maintainer adds `match` to the list above (no stall of `match` can arise
before; once it is listed, `match_a`'s row in the ladder's table applies as any other).

When the judge learns to check a new unit, its author, the project's maintainer, adds the unit's value to this list,
word for word. A value listed here that no task names yet is one the cutter may write a task for, on the file
`units/<unit>.c`. The judge also checks units that are not in this list: the ladder's fallbacks (below), and the staged
units (above). **The cutter writes a task only for a value listed above, and for a fallback only in the cut at a stall
(below), never otherwise.**

A new unit needs the judge's author first: a function of the black box (a new binary, built as `reference/README.md`
says), its entry in the manifest, its cases (`units/UNITS.md` says where they are drawn) and its line in
`units/UNITS.md`. **The cutter writes no task for a unit the judge has no entry for.** An entry is not enough: the unit
must also be listed above.

## The ladder

The project grows by **rungs**: batches of new functions of the black box, each harder than the last, added by the
project's maintainer in one commit (a new binary, the manifest's entries, a stub file per unit, `units/UNITS.md`, this
charter and the backlog). The cutter never adds a rung. A rung's ids go by hundreds: from rung 2 on, rung N's tasks
are `blackbox-0N01` to `-0N49` (N on two digits: `blackbox-0201`, `blackbox-0301`; rung 1's are `blackbox-0001` to
`-0008`), a task cut again taking the next free one; its fallbacks have fixed ids from `-0N51`:

| Rung | Fallback (for) and its fixed id |
| --- | --- |
| 2 | `lz_a` (`lz`) `blackbox-0251`; `vm2_a` (`vm2`) `blackbox-0252` |
| 3 | `kv_a` (`kv`) `blackbox-0351`; `alloc_a` (`alloc`) `blackbox-0352`; `frame_a` (`frame`) `blackbox-0353` |
| 4 | `fold_a` (`fold`) `blackbox-0451`; `match_a` (`match`, staged) `blackbox-0452` |
| 5 | `parse_a` (`parse`) `blackbox-0551`; `run_a` (`run`) `blackbox-0552` |

- **The rung's tasks** are lines of `hive/backlog.jsonl`, published by the cutter as its own rules say (each once the
  tip fails it, and while fewer than `cutter.open` tasks are left to do). Each unit's file is on the tip as a stub
  (the blank unit, which every judgement refuses) so that the tip fails each task until a worker's unit is accepted.
- **A fallback** is the same function of the black box under another name, judged on a narrower family of inputs: the
  table above names them. The judge checks them already; **no task names a fallback until its full task stalls.** A
  task **stalls** when it has had at least four failures (a judged refusal, a deal run out, a deal given back as out of
  reach) and no acceptance, counted from the unit's last cut (a task cut again starts from none: the failures of the
  task it replaces are not its own).
- **The cut at a stall**, one cut, once per unit, only for a unit that has a row in the table above (a stalled task of
  any other unit, or of a unit whose fallback's task exists already, is left to the cutter's ordinary rules). For a unit
  `U` whose fallback is `U_a`, the cut:
  1. closes the stalled task;
  2. adds the fallback's task, under its fixed id: its title « Write the unit U_a: the black box's U, on a narrower
     family of inputs »; its `files` `units/U_a.c` and its option `unit` `U_a`; its context the stalled task's, with
     `U_a` for every `U` in it, and one sentence more: « It is the same function as U, on a narrower family of
     inputs. »; its `needs` a copy of the stalled task's own `needs`, if it had any;
  3. adds a fresh full task for `U`, under a new id, with the stalled task's title, files, option and context, whose
     `parent` is the closed task and whose `needs` are a copy of the closed task's own `needs`, and nothing more: it
     does not wait for the fallback, both are open at once, and a worker takes either;
  4. adds again, under new ids, each task that needed the closed one, needing the fresh task instead (a published line
     never changes).

  The cut is one cut, held to `cutter.changes`; `cutter.open` is the project's aim, not a bound on it.
- **Three rules for every rung:**
  1. **Constants are part of the specification.** The numbers and tables a function computes with may be written in
     a unit as C data, whether found by reading the black box or by running it. What stays forbidden is the black
     box's machine code, as data to decode or interpret, and any emulator of its processor (below).
  2. **The project's own accepted units may be reused.** A unit accepted on the project's `hive` branch is the
     project's own work, under its licence: a unit whose function contains another's may build on it. A copy from
     anywhere else is still forbidden.
     A task's `needs` are met only by the acceptance of the tasks it names: an accepted fallback (`parse_a`, say)
     never meets the `needs` of another unit's task, though it may be read and reused as any accepted unit.
  3. **No task text says how a function works**, at any rung: a title gives the kind of function, as
     `units/UNITS.md` does, and nothing more.

## Never

- A task on a protected path, or with a goal other than « correct » and the option `unit`.
- A task on a unit the judge has no entry for.
- A task naming a fallback (« The ladder »'s table) outside the cut at a stall (« The ladder »).
- A task on a staged unit (`match`, `match_a`) before the project's maintainer lists `match`.
- A task text that says how a function of the black box works: finding out is the task.

## Publishing

The cutter publishes on `main` itself; no person reads each cut before it lands.
