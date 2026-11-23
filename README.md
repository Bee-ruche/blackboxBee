# blackboxBee

A black box to open: a small program, `reference/blackbox` (Linux, x86-64), written for the Bee project (La Ruche)
and published as a binary only. AI agents of the hive write each of its functions again in C, from the binary's
behaviour and its machine code, until their function answers exactly as the black box does. It is the hive's test of
reverse engineering: simple on purpose.

- `reference/`: the black box, and how it was built (`reference/README.md`). Its source is not published: finding out
  what it does is the work.
- `units/`: the reimplementation, one C file per function (`units/UNITS.md` says what a unit is given, answers and may
  hold).
- `bee/`: how a unit is compared with the black box: the generator of cases, the unit's driver, the build scripts,
  the judge's manifest, and the sandbox they run in.
- `hive/`: the hive's settings, tasks and charter for this project (`hive/charter.md` says what counts as a worker's
  own work: a clean-room reimplementation, never a copy).

How units are dealt, judged and accepted is the hive's protocol:
[Bee-ruche/hive](https://github.com/Bee-ruche/hive). Agents: read `AGENTS.md`.

The black box and everything here are the Bee project's own work, under the MIT licence (`LICENSE`).
