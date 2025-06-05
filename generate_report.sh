#!/bin/bash

# Check if the experiment results directory is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <experiment_results_directory>"
    exit 1
fi

TARGET_DIR="$1"
REPORT_FILE="${TARGET_DIR}/statistical_report.txt"

# Check if the provided argument is a directory
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found!"
    exit 2
fi

# --- Report Header ---
echo "--- Real-time Scheduling Experiment Statistical Report ---" > "$REPORT_FILE"
echo "Generated on: $(date)" >> "$REPORT_FILE"
echo "Data from directory: $TARGET_DIR" >> "$REPORT_FILE"
echo "---------------------------------------------------------" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

echo "Generating statistical report..."

# Find all .txt files in the target directory and loop through them
find "$TARGET_DIR" -maxdepth 1 -type f -name "*.txt" | sort | while read -r DATAFILE; do
    FILENAME=$(basename "$DATAFILE")
    echo "Processing statistical data for: $FILENAME"

    # --- Extract metadata from filename ---
    SCHED_POLICY=""
    PARAMS_DESC=""
    STRESS_STATUS=""

    if [[ "$FILENAME" =~ ^(other)_n([0-9-]+)_(no_stress|stress)\.txt$ ]]; then
        SCHED_POLICY=${BASH_REMATCH[1]}
        PARAMS_DESC="Nice: ${BASH_REMATCH[2]}"
        STRESS_STATUS=${BASH_REMATCH[3]/no_stress/No Stress}
    elif [[ "$FILENAME" =~ ^(fifo|rr)_p([0-9]+)_(no_stress|stress)\.txt$ ]]; then
        SCHED_POLICY=${BASH_REMATCH[1]}
        PARAMS_DESC="Priority: ${BASH_REMATCH[2]}"
        STRESS_STATUS=${BASH_REMATCH[3]/no_stress/No Stress}
    elif [[ "$FILENAME" =~ ^(deadline)_R([0-9]+)_D([0-9]+)_(no_stress|stress)\.txt$ ]]; then
        SCHED_POLICY=${BASH_REMATCH[1]}
        RUNTIME_US="${BASH_REMATCH[2]}"
        PERIOD_US_DL="$(( ${BASH_REMATCH[3]} / 1000 ))"
        PARAMS_DESC="Runtime: ${RUNTIME_US}µs, Period/Deadline: ${PERIOD_US_DL}µs"
        STRESS_STATUS=${BASH_REMATCH[4]/no_stress/No Stress}
    else
        echo "WARNING: Unrecognized file pattern for: $FILENAME - Skipping statistical analysis."
        continue
    fi

    # --- Write filename and metadata to report ---
    echo "---------------------------------------------------------" >> "$REPORT_FILE"
    echo "File: $FILENAME" >> "$REPORT_FILE"
    echo "  Scheduler: SCHED_${SCHED_POLICY^^}" >> "$REPORT_FILE"
    echo "  Parameters: $PARAMS_DESC" >> "$REPORT_FILE"
    echo "  Stress Condition: $STRESS_STATUS" >> "$REPORT_FILE"

    # --- Extract and Validate Numeric Data ---
    # Extract only valid numeric data from the first column
    # Using 'grep -E "^[0-9]+(\.[0-9]+)?$"' for strict numeric filtering
    # And then awk to extract the first column
    LATENCY_DATA=$(awk '{print $1}' "$DATAFILE" | grep -E "^[0-9]+(\.[0-9]+)?$")
    
    if [ -z "$LATENCY_DATA" ]; then
        echo "  No valid numeric data found for statistics." >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        continue
    fi

    # --- Calculate Statistics ---
    COUNT=$(echo "$LATENCY_DATA" | wc -l)
    if [ "$COUNT" -eq 0 ]; then # Should not happen if LATENCY_DATA is not empty, but as a safeguard
        echo "  No valid numeric data found for statistics (after filtering)." >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        continue
    fi

    # Using awk for sum, mean, min, max, std dev in one pass for efficiency
    # Gawk specific for sort() and percentiles, if not available, fall back to piping to sort -n
    # Standard awk does not have sort(), so we use pipe to sort -n for percentiles later.
    
    # Calculate Sum, Mean, Min, Max, StdDev (population) in one awk pass
    AWK_STATS=$(echo "$LATENCY_DATA" | awk '
        BEGIN {
            sum = 0;
            count = 0;
            min = 1e18; # Initialize min with a very large number
            max = 0;    # Initialize max with a very small number
        }
        {
            sum += $1;
            count++;
            if ($1 < min) min = $1;
            if ($1 > max) max = $1;
            data[count] = $1; # Store data for std dev calculation in END block
        }
        END {
            if (count == 0) {
                print "Count: 0";
                exit;
            }
            mean = sum / count;
            
            sum_sq_diff = 0;
            for (i=1; i<=count; i++) {
                sum_sq_diff += (data[i] - mean)^2;
            }
            std_dev = sqrt(sum_sq_diff / count); # Population standard deviation

            printf "Count: %d\n", count;
            printf "Sum: %.2f\n", sum;
            printf "Mean: %.2f\n", mean;
            printf "Min: %.2f\n", min;
            printf "Max: %.2f\n", max;
            printf "StdDev: %.2f\n", std_dev;
        }
    ')

    # Assign values from AWK_STATS to shell variables
    COUNT=$(echo "$AWK_STATS" | awk -F': ' '/^Count:/ {print $2}')
    SUM=$(echo "$AWK_STATS" | awk -F': ' '/^Sum:/ {print $2}')
    MEAN=$(echo "$AWK_STATS" | awk -F': ' '/^Mean:/ {print $2}')
    MIN=$(echo "$AWK_STATS" | awk -F': ' '/^Min:/ {print $2}')
    MAX=$(echo "$AWK_STATS" | awk -F': ' '/^Max:/ {print $2}')
    STD_DEV=$(echo "$AWK_STATS" | awk -F': ' '/^StdDev:/ {print $2}')


    # Percentiles (using sort and awk to find specific lines)
    # The `sort -n` is crucial here.
    # We use a custom awk script for percentiles to handle both odd/even counts
    PERCENTILES=$(echo "$LATENCY_DATA" | sort -n | awk -v c="$COUNT" '
        BEGIN {
            if (c > 0) {
                # 50th Percentile (Median)
                idx_50 = c / 2;
                if (c % 2 == 1) { # Odd count
                    line_50 = int(idx_50) + 1;
                } else { # Even count
                    line_50_1 = idx_50;
                    line_50_2 = idx_50 + 1;
                }

                # 90th Percentile
                idx_90 = c * 0.9;
                line_90 = int(idx_90) + 1; # Use floor + 1 for common percentile def
                if (line_90 == 0 && c > 0) line_90 = 1; # Edge case for tiny counts

                # 99th Percentile
                idx_99 = c * 0.99;
                line_99 = int(idx_99) + 1; # Use floor + 1
                if (line_99 == 0 && c > 0) line_99 = 1; # Edge case for tiny counts
            }
        }
        {
            data[NR] = $1;
        }
        END {
            if (c == 0) exit;

            # Median
            if (c % 2 == 1) {
                median = data[line_50];
            } else {
                median = (data[line_50_1] + data[line_50_2]) / 2;
            }
            
            # P90
            p90 = data[line_90];
            if (line_90 > c) p90 = data[c]; # Cap at max if index too high

            # P99
            p99 = data[line_99];
            if (line_99 > c) p99 = data[c]; # Cap at max if index too high


            printf "Median (P50): %.2f\n", median;
            printf "P90: %.2f\n", p90;
            printf "P99: %.2f\n", p99;
        }
    ')

    # Assign values from PERCENTILES to shell variables
    MEDIAN=$(echo "$PERCENTILES" | awk -F': ' '/^Median/ {print $2}')
    P90=$(echo "$PERCENTILES" | awk -F': ' '/^P90/ {print $2}')
    P99=$(echo "$PERCENTILES" | awk -F': ' '/^P99/ {print $2}')

    # --- Write statistics to report ---
    echo "  Count: $COUNT" >> "$REPORT_FILE"
    echo "  Sum: $SUM µs" >> "$REPORT_FILE"
    echo "  Mean: $MEAN µs" >> "$REPORT_FILE"
    echo "  Median (P50): $MEDIAN µs" >> "$REPORT_FILE"
    echo "  Min: $MIN µs" >> "$REPORT_FILE"
    echo "  Max: $MAX µs" >> "$REPORT_FILE"
    echo "  StdDev: $STD_DEV µs" >> "$REPORT_FILE"
    echo "  P90: $P90 µs" >> "$REPORT_FILE"
    echo "  P99: $P99 µs" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"

done

echo "---" >> "$REPORT_FILE"
echo "Statistical report saved to: $REPORT_FILE"
echo "--- Statistical Report Generation Complete ---"