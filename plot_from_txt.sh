#!/bin/bash

# Check if input file is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 datafile.txt"
    exit 1
fi

DATAFILE="$1"
PLOTFILE="${DATAFILE%.*}.png"

# Check if the file exists
if [ ! -f "$DATAFILE" ]; then
    echo "File '$DATAFILE' not found!"
    exit 2
fi

# Create Gnuplot script
GNUPLOT_SCRIPT=$(mktemp)

cat <<EOF > "$GNUPLOT_SCRIPT"
set terminal pngcairo size 800,600 enhanced font 'Verdana,10'
set output '$PLOTFILE'
set title '1D Data Plot from $DATAFILE'
set xlabel 'Index'
set ylabel 'Value'
set grid
plot '$DATAFILE' using 0:1 with linespoints title 'Data'
EOF

# Run Gnuplot
gnuplot "$GNUPLOT_SCRIPT"

# Clean up
rm "$GNUPLOT_SCRIPT"

echo "Plot saved to $PLOTFILE"
