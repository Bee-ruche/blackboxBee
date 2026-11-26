# The tooling of blackboxBee

- `reference/`: what the hive's judge reads of this project (its kind is `reference`: `docs/reference-judge.md` of
  Bee-ruche/hive):
  - `project.json`: the units, their bounds, the image the judge uses (`wasm`: the judge's Ubuntu with gcc, and
    clang 18, its WebAssembly linker and wabt);
  - `build-reference`: copies the black box (`../reference/blackbox`) as the reference, and builds the generator of
    cases (`generate.c`). For the units of rung 2 on, the generator hands the drawing to the black box itself
    (`../reference/blackbox --generate`: `../units/UNITS.md`). The black box is also the reference's driver: it reads a
    file of cases and writes the answers in the judge's framing itself (`../reference/README.md`);
  - `build-unit`: one unit's driver. The unit's file becomes a WebAssembly module that imports nothing (clang,
    `wasm-ld`), read back and held to its rule and its cap (`wasm-objdump`, `wasm-validate`: `../units/UNITS.md`),
    then translated to C by `wasm2c` and compiled with `host.c` and the translator's runtime. The nonce the judge
    gives lays out the module's memory (the host's window, the unit's data, its stack);
  - `host.c`: the driver's own code: the framing, the module's instance, the window, one call per case; a trap ends
    the run. It also gives itself a seccomp filter once the module is ready, an extra the design does not rest on;
  - `mem.c`: the four memory functions every unit's module is given;
  - `unit.h`: the types and the function a unit's file is given;
  - `blank.c`: the blank unit, which the judge builds in a unit's place for its control.

  The judge runs these scripts in containers without network: the reference's container sees the black box and the
  generator; a unit's build sees the skeleton and that unit's file; a unit's run sees its driver and the cases.
- `check.sh`: the quick check of one unit, for who writes it: the same scripts, on cases drawn now.
- `sandbox/`: where project code, the black box included, runs on your machine, and nowhere else.
- `../.github/workflows/judge-check.yml`: on each pull request, the hive's judge asks the tip itself, unit by unit,
  whether the machine works (the manifest, the reference and the generator, the control, cases drawn and framed).

## Sandbox

Two forms, the same as for every project of the hive:

- **The image** (`sandbox/Dockerfile`): the system and the tools the judge uses for this project, and `objdump`.
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
  machine that is not a throwaway). It shows the machine's own `/usr`: it serves this project only where `cc`,
  `clang-18`, `wasm-ld-18` and wabt's tools and runtime live there.
