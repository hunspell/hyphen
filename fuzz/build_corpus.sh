#!/bin/sh
# Build a libFuzzer seed corpus from the existing tests/ data and
# the bundled hyph_en_US.dic. Each seed file has the format:
#
#   [word bytes][NUL][dictionary bytes]
#
# matching what fuzz_hyphen.c expects.

set -eu

cd "$(dirname "$0")/.."

CORPUS_DIR=${1:-fuzz/corpus_seed}
mkdir -p "$CORPUS_DIR"

count=0

# tests/<name>.pat + tests/<name>.word -> one seed per word line
for pat in tests/*.pat; do
    name=$(basename "$pat" .pat)
    wordfile="tests/${name}.word"
    [ -f "$wordfile" ] || continue

    line_no=0
    while IFS= read -r word || [ -n "$word" ]; do
        line_no=$((line_no + 1))
        # strip CR if present
        word=$(printf '%s' "$word" | tr -d '\r')
        out="$CORPUS_DIR/${name}_${line_no}"
        # concat: word, NUL, dictionary
        { printf '%s' "$word"; printf '\0'; cat "$pat"; } > "$out"
        count=$((count + 1))
    done < "$wordfile"
done

# A few generic seeds against hyph_en_US.dic (much larger dictionary,
# helps the FSM walk lots of states).
if [ -f hyph_en_US.dic ]; then
    for w in hyphenation example creating absolutely beautifully \
             antidisestablishmentarianism photograph supercalifragilistic \
             "" "a" "ab" "abc" ".a" "a." ".." "a-b" "12345"; do
        out="$CORPUS_DIR/en_$(printf '%s' "$w" | tr -c 'a-zA-Z0-9' '_')"
        { printf '%s' "$w"; printf '\0'; cat hyph_en_US.dic; } > "$out"
        count=$((count + 1))
    done
fi

# A pure-parser seed: empty word, full hyph_en_US dictionary.
if [ -f hyph_en_US.dic ]; then
    cat hyph_en_US.dic > "$CORPUS_DIR/parser_only_en"
    count=$((count + 1))
fi

# Tiny edge cases for the parser path
{ printf '%s' "x"; printf '\0'; printf 'UTF-8\n1.a\n'; }      > "$CORPUS_DIR/tiny_boundary"
{ printf '%s' "x"; printf '\0'; printf 'UTF-8\n'; }            > "$CORPUS_DIR/tiny_empty_dict"
{ printf '%s' "x"; printf '\0'; printf 'UTF-8\nNOHYPHEN \n1a1\n'; } > "$CORPUS_DIR/tiny_nohyphen_empty"
{ printf '%s' "x"; printf '\0'; printf 'UTF-8\nNEXTLEVEL\nUTF-8\n1a1\n'; } > "$CORPUS_DIR/tiny_nextlevel"
count=$((count + 4))

echo "[corpus] wrote $count seed files to $CORPUS_DIR"
du -sh "$CORPUS_DIR"
