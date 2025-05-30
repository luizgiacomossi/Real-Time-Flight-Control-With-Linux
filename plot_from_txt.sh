#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 datafile.txt"
    exit 1
fi

DATAFILE="$1"
PLOTFILE="${DATAFILE%.*}.pdf"

if [ ! -f "$DATAFILE" ]; then
    echo "File '$DATAFILE' not found!"
    exit 2
fi

GNUPLOT_SCRIPT=$(mktemp)

cat <<EOF > "$GNUPLOT_SCRIPT"
set terminal pdfcairo size 8,6 font 'Verdana,10'
set output '$PLOTFILE'
set title 'Data Plot from $DATAFILE'
set xlabel 'Index'
set ylabel 'Value'
set grid
plot '$DATAFILE' using 0:1 with linespoints title 'Data'
EOF

gnuplot "$GNUPLOT_SCRIPT"

rm "$GNUPLOT_SCRIPT"

echo "Plot saved to $PLOTFILE"
