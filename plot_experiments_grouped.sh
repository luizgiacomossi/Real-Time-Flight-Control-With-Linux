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

# Use a temporary file to store processed base names to avoid duplicate plots
PROCESSED_BASENAMES=$(mktemp)

echo "Searching for experiment files in '$TARGET_DIR'..."

# Find all .txt files and group them by experiment name (without stress/nostress suffix)
find "$TARGET_DIR" -maxdepth 1 -type f -name "*.txt" | while read -r DATAFILE; do
    FILENAME=$(basename "$DATAFILE")
    # Remove .txt and potential _stress or _nostress suffix to get the base experiment name
    BASE_EXPERIMENT_NAME=$(echo "$FILENAME" | sed -E 's/(_stress|_nostress)?\.txt$//')

    # If this base experiment name has already been processed, skip
    if grep -q "^$BASE_EXPERIMENT_NAME$" "$PROCESSED_BASENAMES"; then
        continue
    fi

    echo "$BASE_EXPERIMENT_NAME" >> "$PROCESSED_BASENAMES"

    STRESS_FILE="${TARGET_DIR}/${BASE_EXPERIMENT_NAME}_stress.txt"
    NOSTRESS_FILE="${TARGET_DIR}/${BASE_EXPERIMENT_NAME}_no_stress.txt"
    PLOTFILE="${TARGET_DIR}/${BASE_EXPERIMENT_NAME}_combined.pdf"

    # Initialize plot commands
    PLOT_COMMANDS=""
    TITLE_SUFFIX=""

    # Check for the stress file
    if [ -f "$STRESS_FILE" ]; then
        PLOT_COMMANDS="'$STRESS_FILE' using 0:1 with lines lc rgb 'red' title 'Stress'"
        TITLE_SUFFIX=" (Stress vs. No Stress)"
    fi

    # Check for the no-stress file
    if [ -f "$NOSTRESS_FILE" ]; then
        if [ -n "$PLOT_COMMANDS" ]; then
            PLOT_COMMANDS="${PLOT_COMMANDS}, "
        fi
        PLOT_COMMANDS="${PLOT_COMMANDS}'$NOSTRESS_FILE' using 0:1 with lines lc rgb 'blue' title 'No Stress'"
        TITLE_SUFFIX=" (Stress vs. No Stress)"
    fi

    # If neither file exists for this base name, skip
    if [ -z "$PLOT_COMMANDS" ]; then
        echo "Skipping '$BASE_EXPERIMENT_NAME': No stress or no-stress files found."
        continue
    fi

    echo "Processing '$BASE_EXPERIMENT_NAME'..."

    # Create a temporary Gnuplot script
    GNUPLOT_SCRIPT=$(mktemp)

    cat <<EOF > "$GNUPLOT_SCRIPT"
set terminal pdfcairo size 8,6 font 'Verdana,14'
set output '$PLOTFILE'
set title 'Latency for ${BASE_EXPERIMENT_NAME}${TITLE_SUFFIX}'
set xlabel 'Index'
set ylabel 'Value'
set grid
plot $PLOT_COMMANDS
EOF

    # Execute Gnuplot
    gnuplot "$GNUPLOT_SCRIPT"

    # Remove the temporary script
    rm "$GNUPLOT_SCRIPT"

    echo "Plot saved to $PLOTFILE"
done

# Clean up the temporary file for processed basenames
rm "$PROCESSED_BASENAMES"

echo "---"
echo "All relevant .txt files in '$TARGET_DIR' have been processed."