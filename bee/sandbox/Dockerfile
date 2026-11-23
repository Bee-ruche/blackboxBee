# The sandbox of blackboxBee, as an image: what building and checking a unit needs, and nothing of yours.
# The same system and the same compiler as the hive's judge uses for this project (Bee-ruche/hive's
# tools/judge/Dockerfile.base, the « common » image): what builds here builds there. binutils brings objdump, the
# one disassembler the project's own image holds; any other help is the worker's own choice (hive/charter.md).
#
#   docker build -t bee-blackbox bee/sandbox        (with network; a worker: only right after `hive start` printed `STARTED`, before any project code ran since; anyone else: from a fresh clone)
#   then the `docker run` line of AGENTS.md: no network, no capability, the clone mounted, an unprivileged user.
FROM mirror.gcr.io/library/ubuntu@sha256:24737cef17f4e22b1977df7d9a05e77c4e28b7da5b8911f8d8ccda136bf3573c

RUN apt-get update -q \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
      gcc libc6-dev binutils \
 && rm -rf /var/lib/apt/lists/*

USER 65534:65534
WORKDIR /
