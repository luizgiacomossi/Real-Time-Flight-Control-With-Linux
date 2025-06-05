#!/bin/bash

# Check if the experiment results directory is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <experiment_results_directory>"
    exit 1
fi

TARGET_DIR="$1"

# Check if the provided argument is a directory
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found!"
    exit 2
fi

echo "--- Starting Plot Generation Grouped by Scheduler Policy with Improved Colors ---"
echo "Looking for files in: ${TARGET_DIR}"

# Declare an associative array to store grouped files by scheduler policy
declare -A SCHEDULER_GROUPS

# Use mapfile to read all file paths into an array in the main shell
mapfile -d '' ALL_DATA_FILES < <(find "$TARGET_DIR" -maxdepth 1 -type f -name "*.txt" -print0)

# Categorize files into scheduler groups
for DATAFILE in "${ALL_DATA_FILES[@]}"; do
    FILENAME=$(basename "$DATAFILE")

    SCHED_POLICY=""

    # Extract scheduler policy
    if [[ "$FILENAME" =~ ^(other)_.*\.txt$ ]]; then
        SCHED_POLICY=${BASH_REMATCH[1]}
    elif [[ "$FILENAME" =~ ^(fifo|rr)_.*\.txt$ ]]; then
        SCHED_POLICY=${BASH_REMATCH[1]}
    elif [[ "$FILENAME" =~ ^(deadline)_.*\.txt$ ]]; then
        SCHED_POLICY=${BASH_REMATCH[1]}
    else
        echo "WARNING: Unrecognized file pattern for: $FILENAME - Skipping."
        continue
    fi

    if [ -z "$SCHED_POLICY" ]; then
        echo "ERROR: Could not determine scheduler policy for $FILENAME. Skipping."
        continue
    fi

    # Append the DATAFILE path to the array associated with its scheduler policy
    SCHEDULER_GROUPS["$SCHED_POLICY"]="${SCHEDULER_GROUPS["$SCHED_POLICY"]} $DATAFILE"
done

echo "DEBUG: Finished file categorization. Now processing groups..."

# Define a more structured color palette for consistency
declare -A BASE_COLORS=(
    ["p50"]="web-green"
    ["p99"]="web-blue"
    ["n0"]="orange"
    ["n-19"]="dark-magenta"
    ["R400_D4000"]="cyan"
    ["R800_D4000"]="dark-goldenrod"
)

# Iterate through each scheduler group and generate a combined plot
for SCHED_POLICY in "${!SCHEDULER_GROUPS[@]}"; do
    echo "Processing scheduler group: $SCHED_POLICY"
    OUTPUT_PDF="${TARGET_DIR}/${SCHED_POLICY}_all_combined.pdf"
    PLOT_COMMANDS=""
    FILE_COUNT=0

    # Read the space-separated list of files into an array
    read -r -a FILES_FOR_POLICY <<< "${SCHEDULER_GROUPS[$SCHED_POLICY]}"

    # Sort files to ensure a consistent order for plotting
    IFS=$'\n' sorted_files=($(sort <<<"${FILES_FOR_POLICY[*]}"))
    unset IFS

    for DATAFILE in "${sorted_files[@]}"; do
        if [ -z "$DATAFILE" ]; then
            continue
        fi

        FILENAME=$(basename "$DATAFILE")

        PLOT_TITLE=""
        BASE_COLOR_KEY=""
        STRESS_FLAG=""

        # Determine plot title and base color key based on filename details
        if [[ "$FILENAME" =~ ^(other)_n([0-9-]+)_(no_stress|stress)\.txt$ ]]; then
            PARAMS="n${BASH_REMATCH[2]}"
            STRESS_FLAG=${BASH_REMATCH[3]}
            PLOT_TITLE="Nice ${BASH_REMATCH[2]}"
            BASE_COLOR_KEY="${PARAMS}"
        elif [[ "$FILENAME" =~ ^(fifo|rr)_p([0-9]+)_(no_stress|stress)\.txt$ ]]; then
            PARAMS="p${BASH_REMATCH[2]}"
            STRESS_FLAG=${BASH_REMATCH[3]}
            PLOT_TITLE="${SCHED_POLICY^^} Prio ${BASH_REMATCH[2]}"
            BASE_COLOR_KEY="${PARAMS}"
        elif [[ "$FILENAME" =~ ^(deadline)_R([0-9]+)_D([0-9]+)_(no_stress|stress)\.txt$ ]]; then
            PARAMS="R${BASH_REMATCH[2]}_D${BASH_REMATCH[3]}"
            RUNTIME_US="${BASH_REMATCH[2]}"
            PERIOD_US_DL="$(( ${BASH_REMATCH[3]} / 1000 ))"
            STRESS_FLAG=${BASH_REMATCH[4]}
            PLOT_TITLE="DEADLINE R=${RUNTIME_US}µs D=${PERIOD_US_DL}µs"
            BASE_COLOR_KEY="${PARAMS}"
        else
            echo "WARNING: Could not parse details for plot title for $FILENAME. Skipping."
            continue
        fi

        # Get the base color for the parameter set
        CURRENT_COLOR="${BASE_COLORS["$BASE_COLOR_KEY"]}"
        if [ -z "$CURRENT_COLOR" ]; then
            echo "WARNING: No base color defined for key '$BASE_COLOR_KEY'. Using default 'black'."
            CURRENT_COLOR="black"
        fi

        # Adjust color based on stress status
        if [ "$STRESS_FLAG" == "stress" ]; then
            case "$CURRENT_COLOR" in
                "web-green") FINAL_COLOR="dark-green";;
                "web-blue") FINAL_COLOR="dark-blue";;
                "orange") FINAL_COLOR="dark-orange";;
                "dark-magenta") FINAL_COLOR="dark-magenta";;
                "cyan") FINAL_COLOR="dark-cyan";;
                "dark-goldenrod") FINAL_COLOR="dark-goldenrod";;
                *) FINAL_COLOR="$CURRENT_COLOR";;
            esac
            PLOT_TITLE="${PLOT_TITLE} - Stress"
        else # no_stress
            PLOT_TITLE="${PLOT_TITLE} - No Stress"
            FINAL_COLOR="$CURRENT_COLOR"
        fi

        if [ -n "$PLOT_COMMANDS" ]; then
            PLOT_COMMANDS="${PLOT_COMMANDS}, "
        fi
        PLOT_COMMANDS="${PLOT_COMMANDS}'$DATAFILE' using 0:1 with lines lc rgb '$FINAL_COLOR' title '$PLOT_TITLE'"
        FILE_COUNT=$((FILE_COUNT + 1))
    done

    if [ "$FILE_COUNT" -gt 0 ]; then
        echo "Generating combined plot for scheduler: ${SCHED_POLICY}"
        echo "Output: ${OUTPUT_PDF}"

        GNUPLOT_SCRIPT=$(mktemp)

        cat <<EOF > "$GNUPLOT_SCRIPT"
set terminal pdfcairo size 12,8 font 'Verdana,12'
set output '$OUTPUT_PDF'
set title 'Latency for SCHED_${SCHED_POLICY^^} Policies'
set xlabel 'Job Index'
set ylabel 'Latency (µs)'
set grid
set key inside right top # Changed from 'outside' to 'inside'
plot $PLOT_COMMANDS
EOF

        gnuplot "$GNUPLOT_SCRIPT"
        if [ $? -ne 0 ]; then
            echo "ERROR: Gnuplot failed for ${SCHED_POLICY}. Check Gnuplot output above."
        fi
        rm "$GNUPLOT_SCRIPT"
        echo "Plot saved to $OUTPUT_PDF"
    else
        echo "WARNING: No data files found for scheduler ${SCHED_POLICY} to plot."
    fi
done

echo "--- All plots generated and saved to ${TARGET_DIR} ---"