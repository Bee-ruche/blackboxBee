# The sandbox of blackboxBee, as an image: what building and checking a unit needs, and nothing of yours.
# The same system and the same tools as the hive's judge uses for this project (Bee-ruche/hive's
# tools/judge/Dockerfile.base with the packages of its « wasm » image): what builds here builds there. clang-18 and
# lld-18 build a unit into a WebAssembly module, wabt reads it back and translates it (wasm2c), gcc builds the driver
# and the generator. binutils brings objdump, the one disassembler of the black box the project's own image holds;
# any other help is the worker's own choice (hive/charter.md).
#
#   docker build -t bee-blackbox bee/sandbox        (with network; a worker: only right after `hive start` printed `STARTED`, before any project code ran since; anyone else: from a fresh clone)
#   then the `docker run` line of AGENTS.md: no network, no capability, the clone mounted, an unprivileged user.
FROM mirror.gcr.io/library/ubuntu@sha256:24737cef17f4e22b1977df7d9a05e77c4e28b7da5b8911f8d8ccda136bf3573c

RUN apt-get update -q \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
      gcc libc6-dev binutils clang-18 lld-18 wabt \
 && rm -rf /var/lib/apt/lists/*

USER 65534:65534
WORKDIR /
