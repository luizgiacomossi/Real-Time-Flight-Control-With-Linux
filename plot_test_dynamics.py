import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np

# ---------------------------------------------------------------------------
# Styling
# ---------------------------------------------------------------------------
COLORS = {
    'x':     '#e74c3c',
    'y':     '#2ecc71',
    'z':     '#3498db',
    'roll':  '#e67e22',
    'pitch': '#9b59b6',
    'yaw':   '#1abc9c',
    'ref':   '#95a5a6',
}
WP_COLORS = ['#e74c3c', '#e67e22', '#2ecc71', '#9b59b6']

plt.rcParams.update({
    'font.family':      'DejaVu Sans',
    'axes.spines.top':  False,
    'axes.spines.right':False,
    'axes.grid':        True,
    'grid.alpha':       0.3,
    'legend.fontsize':  12,
    'axes.titlesize':   12,
    'axes.labelsize':   12,
})

# ---------------------------------------------------------------------------
# Shared helpers
# ---------------------------------------------------------------------------
def _add_wp_vlines(axes_list, df, waypoints):
    """Draw vertical waypoint-switch lines on a list of axes."""
    for k in range(1, len(waypoints)):
        wx, wy, wz = waypoints[k]
        switch = df[(df['ref_x'] == wx) & (df['ref_z'] == wz)]['time']
        if not switch.empty:
            t = switch.iloc[0]
            for ax in axes_list:
                ax.axvline(t, color=WP_COLORS[k], lw=1, linestyle=':', alpha=0.8,
                           label=f'→WP{k+1}' if ax is axes_list[0] else None)


def _has_reference(series):
    """Return True only if the reference column contains a meaningful signal
    (i.e., it is not all-zero, which indicates log_state was called with NULL)."""
    return not (series == 0).all()


def _fig_position(df, title, waypoints=None):

    """
    Returns a figure with 3 stacked subplots: X(t), Y(t), Z(t).
    Each subplot shows actual (solid) vs reference (dashed).
    """
    fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
    fig.suptitle(f'{title} — Position Response', fontsize=12, fontweight='bold')

    channels = [
        ('x', 'ref_x', 'X', COLORS['x']),
        ('y', 'ref_y', 'Y', COLORS['y']),
        ('z', 'ref_z', 'Z', COLORS['z']),
    ]
    for ax, (col, ref_col, label, color) in zip(axes, channels):
        ax.plot(df['time'], df[col], color=color, lw=1.8, label=f'{label} actual')
        if _has_reference(df[ref_col]):
            ax.plot(df['time'], df[ref_col], color=COLORS['ref'], lw=1.2,
                    linestyle='--', label=f'{label} ref')
        ax.set_ylabel(f'{label} (m)')
        ax.legend(loc='upper right')

    if waypoints:
        _add_wp_vlines(axes, df, waypoints)
        axes[0].legend(loc='upper right')   # re-draw to include vline labels

    axes[-1].set_xlabel('Time (s)')
    fig.tight_layout()
    return fig


def _fig_attitude(df, title, waypoints=None):
    """
    Returns a figure with 3 stacked subplots: Roll(t), Pitch(t), Yaw(t).
    Angles converted to degrees.
    """
    fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
    fig.suptitle(f'{title} — Attitude Response', fontsize=12, fontweight='bold')

    channels = [
        ('roll',  'Roll',  COLORS['roll']),
        ('pitch', 'Pitch', COLORS['pitch']),
        ('yaw',   'Yaw',   COLORS['yaw']),
    ]
    for ax, (col, label, color) in zip(axes, channels):
        ax.plot(df['time'], df[col] * 180 / np.pi, color=color, lw=1.8, label=label)
        ax.set_ylabel(f'{label} (deg)')
        ax.legend(loc='upper right')

    if waypoints:
        _add_wp_vlines(axes, df, waypoints)

    axes[-1].set_xlabel('Time (s)')
    fig.tight_layout()
    return fig


def _fig_overview(df, title, targets=None, waypoints=None):
    """
    2×2 overview figure:
      top-left  : 3D trajectory
      top-right : position vs time (X, Y, Z overlaid)
      bottom-left: attitude vs time (Roll, Pitch, Yaw overlaid)
      bottom-right: top-down XY view
    """
    fig = plt.figure(figsize=(14, 10))
    fig.suptitle(f'{title} — Overview', fontsize=13, fontweight='bold')

    ax3d  = fig.add_subplot(2, 2, 1, projection='3d')
    ax_pos = fig.add_subplot(2, 2, 2)
    ax_att = fig.add_subplot(2, 2, 3)
    ax_xy  = fig.add_subplot(2, 2, 4)

    # --- 3D trajectory ---
    ax3d.plot(df['x'], df['y'], df['z'], 'b-', lw=1.5, label='Trajectory')
    ax3d.scatter([df['x'].iloc[0]], [df['y'].iloc[0]], [df['z'].iloc[0]],
                 color='k', marker='o', s=60, zorder=5, label='Start')
    for tx, ty, tz in (targets or []):
        ax3d.scatter([tx], [ty], [tz], color='gold', marker='*',
                     s=300, zorder=5, label=f'Target ({tx},{ty},{tz})')
    for k, (wx, wy, wz) in enumerate(waypoints or []):
        ax3d.scatter([wx], [wy], [wz], color=WP_COLORS[k], marker='*',
                     s=200, zorder=5, label=f'WP{k+1}')
        ax3d.text(wx, wy, wz + 0.15, f'WP{k+1}', fontsize=7, color=WP_COLORS[k])
    ax3d.set_xlabel('X (m)'); ax3d.set_ylabel('Y (m)'); ax3d.set_zlabel('Z (m)')
    ax3d.set_title('3D Trajectory')
    ax3d.legend(fontsize=7)

    # --- position vs time ---
    for col, ref_col, label, color in [
        ('x', 'ref_x', 'X', COLORS['x']),
        ('y', 'ref_y', 'Y', COLORS['y']),
        ('z', 'ref_z', 'Z', COLORS['z']),
    ]:
        ax_pos.plot(df['time'], df[col], color=color, lw=1.5, label=label)
        if _has_reference(df[ref_col]):
            ax_pos.plot(df['time'], df[ref_col], color=color, lw=1,
                        linestyle='--', alpha=0.5, label=f'{label} ref')
    if waypoints:
        _add_wp_vlines([ax_pos], df, waypoints)
    ax_pos.set_xlabel('Time (s)'); ax_pos.set_ylabel('Position (m)')
    ax_pos.set_title('Position vs Time'); ax_pos.legend(fontsize=7)

    # --- attitude vs time ---
    for col, label, color in [
        ('roll',  'Roll',  COLORS['roll']),
        ('pitch', 'Pitch', COLORS['pitch']),
        ('yaw',   'Yaw',   COLORS['yaw']),
    ]:
        ax_att.plot(df['time'], df[col] * 180 / np.pi, color=color, lw=1.5, label=label)
    if waypoints:
        _add_wp_vlines([ax_att], df, waypoints)
    ax_att.set_xlabel('Time (s)'); ax_att.set_ylabel('Angle (deg)')
    ax_att.set_title('Attitude vs Time'); ax_att.legend(fontsize=7)

    # --- top-down XY ---
    ax_xy.plot(df['x'], df['y'], 'b-', lw=1.5, label='Trajectory')
    ax_xy.scatter([df['x'].iloc[0]], [df['y'].iloc[0]],
                  color='k', marker='o', s=60, zorder=5, label='Start')
    for tx, ty, tz in (targets or []):
        ax_xy.scatter([tx], [ty], color='gold', marker='*', s=300, zorder=5,
                      label=f'Target ({tx},{ty})')
    if waypoints:
        wp_x = [w[0] for w in waypoints]
        wp_y = [w[1] for w in waypoints]
        ax_xy.plot(wp_x, wp_y, 'k--', lw=1, alpha=0.4, label='WP path')
        for k, (wx, wy, wz) in enumerate(waypoints):
            ax_xy.scatter([wx], [wy], color=WP_COLORS[k], marker='*',
                          s=200, zorder=5, label=f'WP{k+1}')
            ax_xy.annotate(f'WP{k+1}', (wx, wy), textcoords='offset points',
                           xytext=(6, 4), fontsize=8)
    ax_xy.set_xlabel('X (m)'); ax_xy.set_ylabel('Y (m)')
    ax_xy.set_title('Top-Down View (XY)')
    ax_xy.set_aspect('equal'); ax_xy.legend(fontsize=7)

    fig.tight_layout()
    return fig


def _fig_3d(df, title, targets=None, waypoints=None):
    """Standalone 3D trajectory figure."""
    fig = plt.figure(figsize=(8, 7))
    ax = fig.add_subplot(111, projection='3d')
    fig.suptitle(f'{title} — 3D Trajectory', fontsize=12, fontweight='bold')

    ax.plot(df['x'], df['y'], df['z'], 'b-', lw=1.5, label='Trajectory')
    ax.scatter([df['x'].iloc[0]], [df['y'].iloc[0]], [df['z'].iloc[0]],
               color='k', marker='o', s=60, zorder=5, label='Start')
    for tx, ty, tz in (targets or []):
        ax.scatter([tx], [ty], [tz], color='gold', marker='*',
                   s=300, zorder=5, label=f'Target ({tx},{ty},{tz})')
    for k, (wx, wy, wz) in enumerate(waypoints or []):
        ax.scatter([wx], [wy], [wz], color=WP_COLORS[k], marker='*',
                   s=200, zorder=5, label=f'WP{k+1}')
        ax.text(wx, wy, wz + 0.15, f'WP{k+1}', fontsize=8, color=WP_COLORS[k])
    ax.set_xlabel('X (m)'); ax.set_ylabel('Y (m)'); ax.set_zlabel('Z (m)')
    ax.legend(fontsize=8)
    fig.tight_layout()
    return fig


def _fig_topdown(df, title, targets=None, waypoints=None):
    """Standalone top-down XY view figure."""
    fig, ax = plt.subplots(figsize=(7, 7))
    fig.suptitle(f'{title} — Top-Down View (XY)', fontsize=12, fontweight='bold')

    ax.plot(df['x'], df['y'], 'b-', lw=1.5, label='Trajectory')
    ax.scatter([df['x'].iloc[0]], [df['y'].iloc[0]],
               color='k', marker='o', s=60, zorder=5, label='Start')
    for tx, ty, tz in (targets or []):
        ax.scatter([tx], [ty], color='gold', marker='*', s=300, zorder=5,
                   label=f'Target ({tx},{ty})')
    if waypoints:
        wp_x = [w[0] for w in waypoints]
        wp_y = [w[1] for w in waypoints]
        ax.plot(wp_x, wp_y, 'k--', lw=1, alpha=0.4, label='WP path')
        for k, (wx, wy, wz) in enumerate(waypoints):
            ax.scatter([wx], [wy], color=WP_COLORS[k], marker='*',
                       s=200, zorder=5, label=f'WP{k+1}')
            ax.annotate(f'WP{k+1}', (wx, wy), textcoords='offset points',
                        xytext=(6, 4), fontsize=9)
    ax.set_xlabel('X (m)'); ax.set_ylabel('Y (m)')
    ax.set_aspect('equal'); ax.legend(fontsize=8)
    fig.tight_layout()
    return fig


def _save(fig, path):
    fig.savefig(path, bbox_inches='tight')  # SVG: vector, no dpi needed
    plt.close(fig)
    print(f"  Saved {path}")


# ---------------------------------------------------------------------------
# Per-scenario plot functions
# ---------------------------------------------------------------------------
def plot_scenario(csv_file, stem, title, targets=None):
    """
    Generates 5 figures per scenario:
      <stem>_overview.svg  — 2×2 combined overview
      <stem>_3d.svg        — 3D trajectory alone
      <stem>_topdown.svg   — top-down XY alone
      <stem>_position.svg  — X / Y / Z each in their own subplot
      <stem>_attitude.svg  — Roll / Pitch / Yaw each in their own subplot
    """
    if not os.path.exists(csv_file):
        print(f"  [SKIP] {csv_file} not found.")
        return

    tgts = targets or []
    df = pd.read_csv(csv_file)
    _save(_fig_overview(df, title, targets=tgts),  f'{stem}_overview.svg')
    _save(_fig_3d(df, title, targets=tgts),        f'{stem}_3d.svg')
    _save(_fig_topdown(df, title, targets=tgts),   f'{stem}_topdown.svg')
    _save(_fig_position(df, title),                f'{stem}_position.svg')
    _save(_fig_attitude(df, title),                f'{stem}_attitude.svg')


def plot_waypoints(csv_file, stem, out_dir=".", title='Test 5 – 4-Waypoint Sequence', waypoints=None):
    """
    Generates figures for the 4-waypoint scenario, with vertical
    waypoint-switch markers on each subplot.
    """
    if waypoints is None:
        waypoints = [
            (0.0, 0.0, 3.0),
            (3.0, 0.0, 3.0),
            (3.0, 3.0, 5.0),
            (0.0, 0.0, 1.0),
        ]

    if not os.path.exists(csv_file):
        print(f"  [SKIP] {csv_file} not found.")
        return

    os.makedirs(out_dir, exist_ok=True)

    df = pd.read_csv(csv_file)
    _save(_fig_overview(df, title, waypoints=waypoints), os.path.join(out_dir, f'{stem}_overview.svg'))
    _save(_fig_3d(df, title, waypoints=waypoints), os.path.join(out_dir, f'{stem}_3d.svg'))
    _save(_fig_topdown(df, title, waypoints=waypoints), os.path.join(out_dir, f'{stem}_topdown.svg'))
    _save(_fig_position(df, title, waypoints=waypoints), os.path.join(out_dir, f'{stem}_position.svg'))
    _save(_fig_attitude(df, title, waypoints=waypoints), os.path.join(out_dir, f'{stem}_attitude.svg'))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    scenarios = [
        ('test_hover.csv',  'test_hover',  'Test 1 – Hover (open-loop)',                  []),
        ('test_roll.csv',   'test_roll',   'Test 2 – Roll (τx=0.01 N·m, open-loop)',      []),
        ('test_climb.csv',  'test_climb',  'Test 3 – Climb to Z=5 m',                    [(0, 0, 5)]),
        ('test_hard.csv',   'test_hard',   'Test 4 – Complex (X=2, Y=3, Z=10)',           [(2, 3, 10)]),
    ]

    for csv_file, stem, title, targets in scenarios:
        print(f'[{stem}]')
        plot_scenario(csv_file, stem, title, targets=targets)

    print('[waypoints]')
    plot_waypoints('test_waypoints.csv', 'test_waypoints')

    print('All plots saved.')


if __name__ == '__main__':
    main()
