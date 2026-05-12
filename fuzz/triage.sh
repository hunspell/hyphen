#!/bin/sh
# Group crash artifacts by ASan stack-trace signature.
#
# For each file in $ARTIFACTS, replay it through fuzz_hyphen, extract a
# short signature from the ASan output (top 3 frames + error type), and
# group by signature. Print one representative input + count per group.

set -eu

cd "$(dirname "$0")/.."

ARTIFACTS=${1:-/home/caolan/auto/jail/tmp/hyphen-fuzz/artifacts}
OUT=${2:-/home/caolan/auto/jail/tmp/hyphen-fuzz/triage}

mkdir -p "$OUT/by_sig"
: > "$OUT/index.txt"

total=0
for f in "$ARTIFACTS"/crash-* "$ARTIFACTS"/leak-* "$ARTIFACTS"/oom-* "$ARTIFACTS"/timeout-*; do
    [ -f "$f" ] || continue
    total=$((total + 1))

    # Replay quickly. Symbolised ASan output goes to stderr.
    out=$(ASAN_OPTIONS=detect_leaks=1:abort_on_error=0:symbolize=1 \
          UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0:symbolize=1 \
          timeout 15 ./fuzz/fuzz_hyphen "$f" 2>&1 || true)

    # Extract error kind. Try ASan first ("ERROR: AddressSanitizer: foo"),
    # fall back to UBSan ("hyphen.c:N:N: runtime error: foo"), then
    # libFuzzer ("ERROR: libFuzzer: foo").
    err=$(printf '%s\n' "$out" \
          | grep -oE 'ERROR: AddressSanitizer: [a-zA-Z-]+' \
          | head -1 \
          | sed 's/.*: //')
    if [ -z "$err" ]; then
        err=$(printf '%s\n' "$out" \
              | grep -oE 'runtime error: [a-zA-Z][a-zA-Z -]*' \
              | head -1 \
              | sed 's/runtime error: //; s/ *$//; s/ /-/g')
        [ -n "$err" ] && err="ubsan-${err}"
    fi
    if [ -z "$err" ]; then
        err=$(printf '%s\n' "$out" \
              | grep -oE 'ERROR: libFuzzer: [a-zA-Z-]+' \
              | head -1 \
              | sed 's/.*: //')
        [ -n "$err" ] && err="libfuzzer-${err}"
    fi
    [ -n "$err" ] || err="unknown"

    frames=$(printf '%s\n' "$out" \
             | grep -oE 'in (hnj_[a-zA-Z0-9_]+|LLVMFuzzerTestOneInput|run_hyphenate)' \
             | sed 's/^in //' \
             | head -3 \
             | tr '\n' '|' \
             | sed 's/|$//')

    line=$(printf '%s\n' "$out" \
           | grep -oE 'hyphen\.c:[0-9]+' \
           | head -1)

    sig="${err}:${frames}:${line}"
    sig_safe=$(printf '%s' "$sig" | tr '/:|' '___' | tr -c 'A-Za-z0-9_.-' '_')
    mkdir -p "$OUT/by_sig/$sig_safe"
    cp "$f" "$OUT/by_sig/$sig_safe/"
    echo "$sig	$f" >> "$OUT/index.txt"
done

echo
echo "[triage] processed $total artifacts"
echo
echo "[triage] groups (signature - count - example):"
sort "$OUT/index.txt" | awk -F'\t' '
    { count[$1]++; example[$1]=$2 }
    END { for (s in count) printf "  %4d  %s  (eg. %s)\n", count[s], s, example[s] }
' | sort -rn -k1,1
