# The tooling of blackboxBee

- `reference/`: what the hive's judge reads of this project (its kind is `reference`: `docs/reference-judge.md` of
  Bee-ruche/hive):
  - `project.json`: the units, their bounds, the image the judge uses (`common`: the judge's Ubuntu with gcc);
  - `build-reference`: copies the black box (`../reference/blackbox`) as the reference, and builds the generator of
    cases (`generate.c`). The black box is also the reference's driver: it reads a file of cases and writes the
    answers in the judge's framing itself (`../reference/README.md`);
  - `build-unit`: one unit's driver (`unit_driver.c`, `unit.h`, the unit's file), with the nonce the judge gives
    laying two paddings around the unit. It also holds the project's rule on what a unit may be
    (`../units/UNITS.md`): no preprocessor, none of the words that reach past the C language, no call to anything
    outside the unit. The driver then calls the unit under a seccomp filter of its own (read and write on what is
    already open, and end; nothing else): the second wall;
  - `blank.c`: the blank unit, which the judge builds in a unit's place for its control.

  The judge runs these scripts in containers without network: the reference's container sees the black box and the
  generator; a unit's build sees the skeleton and that unit's file; a unit's run sees its driver and the cases.
- `check.sh`: the quick check of one unit, for who writes it: the same scripts, on cases drawn now.
- `sandbox/`: where project code, the black box included, runs on your machine, and nowhere else.
- `../.github/workflows/judge-check.yml`: on each pull request, the hive's judge asks the tip itself, unit by unit,
  whether the machine works (the manifest, the reference and the generator, the control, cases drawn and framed).

## Sandbox

Two forms, the same as for every project of the hive:

- **The image** (`sandbox/Dockerfile`): the system and the compiler the judge uses for this project, and `objdump`.
  `AGENTS.md` gives the two commands: build the image (with network; it holds nothing of the project), then run the
  check in it without network, the clone mounted read-only.
- **Without Docker** (`sandbox/sans-docker.sh <command>`, Linux): no network, none of your environment, an empty
  home; the work tree is the one writable place, but for its guarded paths, read-only there: `.git` (its history
  only), `.github`, `.claude`, `.mcp.json`, `AGENTS.md`, `CLAUDE.md`, what the project's settings protect (`bee/`,
  `reference/`, `units/UNITS.md`, `README.md`, `LICENSE`, `.gitignore`), the settings (`hive/`) and `.gitattributes`.
  A unit, `units/<unit>.c`, is the command's to write. The check writes in the sandbox's own `/tmp` only. After a run,
  treat what stays writable as untrusted: the rest of the tree, and any new path (an instructions file for an agent
  among them). `sandbox/test_sandbox.sh sandbox/sans-docker.sh <an empty folder>` checks each protection on a
  throwaway repository it makes there (read its first lines before running it as root; never for a worker, never on a
  machine that is not a throwaway). It shows the machine's own `/usr`: it serves this project only where `cc` and
  `nm` live there.
