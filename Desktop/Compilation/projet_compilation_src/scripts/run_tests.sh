#!/usr/bin/env bash
set -euo pipefail
MINICC=./minicc

for f in Tests/Syntaxe/OK/*.c; do $MINICC -s "$f" >/dev/null; done
for f in Tests/Syntaxe/KO/*.c; do ! $MINICC -s "$f" >/dev/null 2>&1; done

for f in Tests/Verif/OK/*.c; do $MINICC -v "$f" >/dev/null; done
for f in Tests/Verif/KO/*.c; do ! $MINICC -v "$f" >/dev/null 2>&1; done

for f in Tests/Gencode/OK/*.c; do $MINICC "$f" >/dev/null; test -s out.s; done
for f in Tests/Gencode/KO/*.c; do $MINICC "$f" >/dev/null; test -s out.s; done

echo "OK"
