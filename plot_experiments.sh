#!/bin/bash

# Check if a directory is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

TARGET_DIR="$1"

# Check if the provided argument is a directory
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found!"
    exit 2
fi

# Find all .txt files in the target directory and loop through them
find "$TARGET_DIR" -maxdepth 1 -type f -name "*.txt" | while read -r DATAFILE; do
    # Extract the base name for the plot file
    FILENAME=$(basename "$DATAFILE")
    PLOTFILE="${DATAFILE%.*}.pdf"

    echo "Processing '$FILENAME'..."

    # Create a temporary Gnuplot script
    GNUPLOT_SCRIPT=$(mktemp)

    cat <<EOF > "$GNUPLOT_SCRIPT"
set terminal pdfcairo size 8,6 font 'Verdana,14'
set output '$PLOTFILE'
set title 'Latency from $FILENAME'
set xlabel 'Index'
set ylabel 'Value'
set grid
plot '$DATAFILE' using 0:1 with lines title 'Data'
EOF

    # Execute Gnuplot
    gnuplot "$GNUPLOT_SCRIPT"

    # Remove the temporary script
    rm "$GNUPLOT_SCRIPT"

    echo "Plot saved to $PLOTFILE"
done

echo "---"
echo "All .txt files in '$TARGET_DIR' have been processed."