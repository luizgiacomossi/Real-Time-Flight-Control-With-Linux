import matplotlib.pyplot as plt
import re

with open("statistical_report.txt", "r") as f:
    content = f.read()

# Split blocks by divider, and clean empty parts
blocks = [b.strip() for b in content.split("---------------------------------------------------------") if "File:" in b]

results = []

for block in blocks:
    def extract(pattern):
        match = re.search(pattern, block)
        return float(match.group(1)) if match else None

    file_match = re.search(r"File:\s*(.+)", block)
    if not file_match:
        continue
    filename = file_match.group(1).replace(".txt", "")

    stats = {
        "label": filename,
        "min": extract(r"Min:\s*([\d.]+)"),
        "p50": extract(r"Median\s+\(P50\):\s*([\d.]+)"),
        "p90": extract(r"P90:\s*([\d.]+)"),
        "p99": extract(r"P99:\s*([\d.]+)"),
        "max": extract(r"Max:\s*([\d.]+)"),
    }
    print(f"Extracted stats for {filename}: {stats}")

    if all(stats.values()):
        results.append(stats)

# Sort for better visual grouping
results.sort(key=lambda x: x["label"])

# Plotting
fig, ax = plt.subplots(figsize=(18, 10))
y_pos = range(len(results))

for i, r in enumerate(results):
    y = i + 1

    # Whiskers: min to max
    ax.plot([r["min"], r["max"]], [y, y], color='lightgray', linewidth=4, zorder=1)

    # "Box": p90 to p99
    ax.plot([r["p90"], r["p99"]], [y, y], color='orange', linewidth=10, solid_capstyle='butt', zorder=2)

    # Median
    ax.plot(r["p50"], y, 'ko', label='P50' if i == 0 else "", zorder=3)

    # Label
    ax.text(r["max"] + 30, y, r["label"], verticalalignment='center', fontsize=8)

ax.set_yticks([])
ax.set_xlabel("Latency (µs)")
ax.set_title("Summary of Scheduling Latency: Min–Max | P90–P99 | Median")
ax.grid(axis='x', linestyle='--', alpha=0.5)
plt.tight_layout()
plt.show()
