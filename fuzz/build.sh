#!/bin/sh
# Build libFuzzer harnesses for hyphen.
#
# Produces two binaries in fuzz/:
#   fuzz_hyphen      - libFuzzer + ASan + UBSan, the actual fuzz target.
#   fuzz_hyphen_cov  - coverage-instrumented (no sanitizers), for post-run
#                      coverage reporting via llvm-profdata + llvm-cov.

set -eu

cd "$(dirname "$0")/.."

CC=${CC:-clang}
CFLAGS_COMMON="-O1 -g -fno-omit-frame-pointer -I."
SOURCES="hyphen.c hnjalloc.c fuzz/fuzz_hyphen.c"

echo "[fuzz] building fuzz_hyphen (fuzzer + ASan + UBSan)"
$CC $CFLAGS_COMMON \
    -fsanitize=fuzzer,address,undefined \
    -fno-sanitize-recover=undefined \
    $SOURCES \
    -o fuzz/fuzz_hyphen

echo "[fuzz] building fuzz_hyphen_cov (coverage-instrumented)"
$CC $CFLAGS_COMMON \
    -fsanitize=fuzzer \
    -fprofile-instr-generate -fcoverage-mapping \
    $SOURCES \
    -o fuzz/fuzz_hyphen_cov

echo "[fuzz] done:"
ls -lh fuzz/fuzz_hyphen fuzz/fuzz_hyphen_cov
