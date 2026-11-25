#!/bin/bash
mkdir -p src/data

for infile in data/*; do
    [ -f "$infile" ] || continue  # skip non-files
    filename=$(basename -- "$infile")
    stem="data_${filename%.*}"
    outfile="src/data/${stem}.c"

    echo "Converting $infile -> $outfile (var name: $stem)"
    ./convert "$infile" "$stem" "$outfile"
done

echo "Done."

