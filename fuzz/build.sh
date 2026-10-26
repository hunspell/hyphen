#!/bin/sh
# Build libFuzzer harnesses for hyphen.
#
# Produces these binaries in fuzz/:
#   fuzz_hyphen        - libFuzzer + ASan + UBSan, the main fuzz target.
#   fuzz_hyphen_msan   - libFuzzer + MSan, for reads of uninitialized memory.
#   fuzz_hyphen_cov    - coverage-instrumented (no sanitizers), for post-run
#                        coverage reporting via llvm-profdata + llvm-cov.
#   fuzz_substrings    - libFuzzer + ASan + UBSan for the substrings converter.

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

echo "[fuzz] building fuzz_hyphen_msan (fuzzer + MSan)"
$CC $CFLAGS_COMMON \
    -fsanitize=fuzzer,memory -fsanitize-memory-track-origins \
    $SOURCES \
    -o fuzz/fuzz_hyphen_msan

echo "[fuzz] building fuzz_hyphen_cov (coverage-instrumented)"
$CC $CFLAGS_COMMON \
    -fsanitize=fuzzer \
    -fprofile-instr-generate -fcoverage-mapping \
    $SOURCES \
    -o fuzz/fuzz_hyphen_cov

echo "[fuzz] building fuzz_substrings (fuzzer + ASan + UBSan)"
$CC $CFLAGS_COMMON \
    -fsanitize=fuzzer,address,undefined \
    -fno-sanitize-recover=undefined \
    fuzz/fuzz_substrings.c \
    -o fuzz/fuzz_substrings

echo "[fuzz] done:"
ls -lh fuzz/fuzz_hyphen fuzz/fuzz_hyphen_msan fuzz/fuzz_hyphen_cov fuzz/fuzz_substrings
