#!/bin/sh
# Long fuzz run launcher.
#
# Tunables (env):
#   FORKS         - number of parallel fork workers (default 8)
#   MAX_TIME      - max_total_time in seconds (default 43200 = 12h)
#   FUZZ_STATE    - directory holding corpus/, artifacts/, logs/

set -eu

cd "$(dirname "$0")/.."

FORKS=${FORKS:-8}
MAX_TIME=${MAX_TIME:-43200}
FUZZ_STATE=${FUZZ_STATE:-/home/caolan/auto/jail/tmp/hyphen-fuzz}

mkdir -p "$FUZZ_STATE/corpus" "$FUZZ_STATE/artifacts" "$FUZZ_STATE/logs"

LOG="$FUZZ_STATE/logs/fuzz.log"
echo "[run_long] starting at $(date -Iseconds)" | tee -a "$LOG"
echo "[run_long]   forks=$FORKS max_time=${MAX_TIME}s state=$FUZZ_STATE" | tee -a "$LOG"

ASAN_OPTIONS="detect_leaks=0:abort_on_error=1:dedup_token_length=3:print_stacktrace=1:symbolize=1" \
UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0:symbolize=1" \
exec ./fuzz/fuzz_hyphen \
    -fork="$FORKS" \
    -ignore_crashes=1 \
    -ignore_timeouts=1 \
    -ignore_ooms=1 \
    -timeout=25 \
    -rss_limit_mb=2048 \
    -max_len=8192 \
    -max_total_time="$MAX_TIME" \
    -reload=30 \
    -print_final_stats=1 \
    -artifact_prefix="$FUZZ_STATE/artifacts/" \
    "$FUZZ_STATE/corpus" \
    >> "$LOG" 2>&1
