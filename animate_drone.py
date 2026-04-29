import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter
import argparse
import os

# ---------------------------------------------------------------------------
# Scenario definitions
# ---------------------------------------------------------------------------
SCENARIOS = [
    {
        "name":    "hover",
        "csv":     "test_hover.csv",
        "gif":     "anim_hover.gif",
        "title":   "Test 1 – Hover (open-loop)",
        "xlim":    (-1, 1), "ylim": (-1, 1), "zlim": (0, 2),
        "targets": [],          # no reference target
        "waypoints": [],
        "fps":     20,
        "step":    1,           # keep every frame (1-second sim)
    },
    {
        "name":    "roll",
        "csv":     "test_roll.csv",
        "gif":     "anim_roll.gif",
        "title":   "Test 2 – Roll (open-loop, τx=0.01 N·m)",
        "xlim":    (-1, 1), "ylim": (-1, 1), "zlim": (-2, 2),
        "targets": [],
        "waypoints": [],
        "fps":     20,
        "step":    1,
    },
    {
        "name":    "climb",
        "csv":     "test_climb.csv",
        "gif":     "anim_climb.gif",
        "title":   "Test 3 – Climb to Z=5 m",
        "xlim":    (-1, 1), "ylim": (-1, 1), "zlim": (0, 6),
        "targets": [(0, 0, 5)],
        "waypoints": [],
        "fps":     20,
        "step":    5,
    },
    {
        "name":    "hard",
        "csv":     "test_hard.csv",
        "gif":     "anim_hard.gif",
        "title":   "Test 4 – Complex (X=2, Y=3, Z=10)",
        "xlim":    (-1, 3), "ylim": (-1, 4), "zlim": (0, 12),
        "targets": [(2, 3, 10)],
        "waypoints": [],
        "fps":     20,
        "step":    5,
    },
    {
        "name":    "waypoints",
        "csv":     "test_waypoints.csv",
        "gif":     "anim_waypoints.gif",
        "title":   "Test 5 – 4-Waypoint Sequence",
        "xlim":    (-0.5, 4), "ylim": (-0.5, 4), "zlim": (0, 6),
        "targets": [],
        "waypoints": [
            (0.0, 0.0, 3.0),
            (3.0, 0.0, 3.0),
            (3.0, 3.0, 5.0),
            (0.0, 0.0, 1.0),
        ],
        "fps":     20,
        "step":    5,
    },
]

WP_COLORS = ["#e74c3c", "#e67e22", "#2ecc71", "#9b59b6"]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def get_rotation_matrix(roll, pitch, yaw):
    # The dynamics model uses non-standard sign conventions for both roll and pitch:
    #   positive roll  → +Y acceleration  (opposite to standard ZYX)
    #   positive pitch → -X acceleration, nose-up (opposite to standard ZYX)
    # Negating both angles before feeding into the standard ZYX matrix makes
    # the visual tilt consistent with the physical motion.
    cphi, sphi = np.cos(-roll),  np.sin(-roll)
    cth,  sth  = np.cos(-pitch), np.sin(-pitch)
    cpsi, spsi = np.cos(yaw),    np.sin(yaw)
    return np.array([
        [cpsi*cth, cpsi*sth*sphi - spsi*cphi, cpsi*sth*cphi + spsi*sphi],
        [spsi*cth, spsi*sth*sphi + cpsi*cphi, spsi*sth*cphi - cpsi*sphi],
        [-sth,     cth*sphi,                  cth*cphi],
    ])


def active_waypoint_idx(ref_x, ref_y, ref_z, waypoints):
    """Return the index of the waypoint currently active, based on ref columns."""
    for k, (wx, wy, wz) in enumerate(waypoints):
        if abs(ref_x - wx) < 1e-3 and abs(ref_y - wy) < 1e-3 and abs(ref_z - wz) < 1e-3:
            return k
    return 0


# ---------------------------------------------------------------------------
# Animate one scenario
# ---------------------------------------------------------------------------
def animate_scenario(sc):
    csv_path = sc["csv"]
    if not os.path.exists(csv_path):
        print(f"  [SKIP] {csv_path} not found – run test_dynamics first.")
        return

    print(f"  Loading {csv_path}…")
    df = pd.read_csv(csv_path)
    df = df.iloc[:: sc["step"]].reset_index(drop=True)

    fig = plt.figure(figsize=(10, 8))
    ax  = fig.add_subplot(111, projection="3d")

    ax.set_xlim(*sc["xlim"])
    ax.set_ylim(*sc["ylim"])
    ax.set_zlim(*sc["zlim"])
    ax.set_xlabel("X (m)")
    ax.set_ylabel("Y (m)")
    ax.set_zlabel("Z (m)")
    ax.set_title(sc["title"])

    L    = 0.4
    arm1 = np.array([[L, 0, 0], [-L, 0, 0]]).T
    arm2 = np.array([[0, L, 0], [0, -L, 0]]).T

    # Static target markers
    for (tx, ty, tz) in sc["targets"]:
        ax.scatter([tx], [ty], [tz], color="gold", marker="*", s=300,
                   label=f"Target ({tx},{ty},{tz})", zorder=5)

    # Static waypoint markers
    wps = sc["waypoints"]
    wp_scatters = []
    for k, (wx, wy, wz) in enumerate(wps):
        sc_pt = ax.scatter([wx], [wy], [wz], color=WP_COLORS[k],
                           marker="*", s=200, zorder=5,
                           label=f"WP{k+1} ({wx},{wy},{wz})")
        ax.text(wx, wy, wz + 0.15, f"WP{k+1}", fontsize=8, color=WP_COLORS[k])
        wp_scatters.append(sc_pt)

    # Active waypoint highlight (animated ring)
    active_ring = ax.scatter([], [], [], s=400, facecolors="none",
                             edgecolors="white", linewidths=2, zorder=6)

    line_traj, = ax.plot([], [], [], "b-", alpha=0.35, label="Trajectory")
    line_arm1, = ax.plot([], [], [], "r-", linewidth=4, label="Arm X")
    line_arm2, = ax.plot([], [], [], "g-", linewidth=4, label="Arm Y")

    ax.legend(loc="upper left", fontsize=7)

    def update(frame):
        line_traj.set_data(df["x"].iloc[:frame], df["y"].iloc[:frame])
        line_traj.set_3d_properties(df["z"].iloc[:frame])

        row = df.iloc[frame]
        pos = np.array([[row["x"]], [row["y"]], [row["z"]]])
        R   = get_rotation_matrix(row["roll"], row["pitch"], row["yaw"])

        a1w = R @ arm1 + pos
        a2w = R @ arm2 + pos

        line_arm1.set_data(a1w[0, :], a1w[1, :])
        line_arm1.set_3d_properties(a1w[2, :])
        line_arm2.set_data(a2w[0, :], a2w[1, :])
        line_arm2.set_3d_properties(a2w[2, :])

        # Move active-waypoint ring to current target
        if wps:
            idx = active_waypoint_idx(row["ref_x"], row["ref_y"], row["ref_z"], wps)
            wx, wy, wz = wps[idx]
            active_ring._offsets3d = ([wx], [wy], [wz])
            active_ring.set_edgecolors(WP_COLORS[idx])

        ax.set_title(
            f"{sc['title']}\n"
            f"t={row['time']:.2f}s  pos=({row['x']:.2f}, {row['y']:.2f}, {row['z']:.2f})"
        )
        artists = [line_traj, line_arm1, line_arm2, active_ring]
        return artists

    print(f"  Generating animation ({len(df)} frames @ {sc['fps']} fps)…")
    ani = FuncAnimation(fig, update, frames=len(df), blit=False)

    print(f"  Saving {sc['gif']}…")
    ani.save(sc["gif"], writer=PillowWriter(fps=sc["fps"]))
    plt.close(fig)
    print(f"  ✓ Saved {sc['gif']}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Drone Animation Generator")
    parser.add_argument("--csv", type=str, help="Path to the flight log CSV")
    parser.add_argument("--gif", type=str, help="Path to output GIF")
    parser.add_argument("--title", type=str, default="Flight Animation", help="Title for the animation")
    parser.add_argument("--waypoints", type=str, help="Path to waypoints.txt (optional)")
    args = parser.parse_args()

    if args.csv and args.gif:
        print(f"Animating custom log: {args.csv}")
        sc = {
            "name": "custom",
            "csv": args.csv,
            "gif": args.gif,
            "title": args.title,
            "fps": 20,
            "targets": [],
            "waypoints": [],
            "step": 5,
                "xlim": (-5, 5),
                "ylim": (-5, 5),
                "zlim": (0, 10),
            "waypoints_file": args.waypoints
        }
        animate_scenario(sc)
    else:
        print("=== Drone Animation Generator (Standard Scenarios) ===")
        for sc in SCENARIOS:
            print(f"\n[{sc['name'].upper()}]")
            animate_scenario(sc)
        print("\nAll done.")

if __name__ == "__main__":
    main()
