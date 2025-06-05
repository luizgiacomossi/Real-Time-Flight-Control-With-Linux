#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

TARGET_DIR="$1"

if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found!"
    exit 2
fi

# --- Histogram Configuration (you can adjust these values) ---
BIN_WIDTH=1.0  # The width of each histogram bin. Adjust based on your data's range and desired granularity.
# ------------------------------------------------------------

find "$TARGET_DIR" -maxdepth 1 -type f -name "*.txt" | while read -r DATAFILE; do
    # Extract the base name (e.g., "experiment1")
    BASE_NAME="${DATAFILE%.*}"
    # Construct the new plot filename with "_hist" added
    PLOTFILE="${BASE_NAME}_hist.pdf"
    
    # Get just the filename for display in the plot title
    FILENAME=$(basename "$DATAFILE")

    echo "Processing '$FILENAME' as a histogram, saving to $(basename "$PLOTFILE")..."

    GNUPLOT_SCRIPT=$(mktemp)

    cat <<EOF > "$GNUPLOT_SCRIPT"
set terminal pdfcairo size 8,6 font 'Verdana,10'
set output '$PLOTFILE'
set title 'Histogram of Data from $FILENAME (Bin Width: ${BIN_WIDTH})'
set xlabel 'Value Range'
set ylabel 'Frequency / Count'
set grid y
set tics out nomirror

# Set histogram style
set style data histograms
set style fill solid 0.6 border rgb "black"
set boxwidth ${BIN_WIDTH} relative

# Define the binning function
bin(x) = floor(x / ${BIN_WIDTH}) * ${BIN_WIDTH} + ${BIN_WIDTH} / 2.0

# Plot the data, assuming it's in the first column of your .txt file
plot '$DATAFILE' using (bin(\$1)):(1) smooth frequency with boxes title 'Data Frequency'
EOF

    gnuplot "$GNUPLOT_SCRIPT"

    rm "$GNUPLOT_SCRIPT"

    echo "Histogram saved to $PLOTFILE"
done

echo "---"
echo "All .txt files in '$TARGET_DIR' have been processed as histograms."