#!/bin/sh
# Replay a single artifact through fuzz_hyphen and emit one line:
#   <signature>\t<path>
# Designed to be invoked from xargs -P.

f=$1
[ -f "$f" ] || exit 0

out=$(ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:symbolize=1 \
      UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0:symbolize=1 \
      timeout 15 /home/caolan/auto/jail/repos/hyphen/fuzz/fuzz_hyphen "$f" 2>&1 || true)

err=$(printf '%s\n' "$out" | grep -oE 'ERROR: AddressSanitizer: [a-zA-Z-]+' | head -1 | sed 's/.*: //')
if [ -z "$err" ]; then
    err=$(printf '%s\n' "$out" | grep -oE 'runtime error: [a-zA-Z][a-zA-Z -]*' | head -1 | sed 's/runtime error: //; s/ *$//; s/ /-/g')
    [ -n "$err" ] && err="ubsan-${err}"
fi
if [ -z "$err" ]; then
    err=$(printf '%s\n' "$out" | grep -oE 'ERROR: libFuzzer: [a-zA-Z-]+' | head -1 | sed 's/.*: //')
    [ -n "$err" ] && err="libfuzzer-${err}"
fi
[ -n "$err" ] || err="unknown"

frames=$(printf '%s\n' "$out" \
         | grep -oE 'in (hnj_[a-zA-Z0-9_]+|LLVMFuzzerTestOneInput|run_hyphenate)' \
         | sed 's/^in //' | head -3 | tr '\n' '|' | sed 's/|$//')

line=$(printf '%s\n' "$out" | grep -oE 'hyphen\.c:[0-9]+' | head -1)

printf '%s:%s:%s\t%s\n' "$err" "$frames" "$line" "$f"
