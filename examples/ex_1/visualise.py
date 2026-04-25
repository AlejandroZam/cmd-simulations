"""Visualise ex_1 missile intercept simulation output.

Usage:
    python visualise.py [output_dir]

output_dir defaults to the build output next to this script.
"""
import sys
import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

# ── Locate output ─────────────────────────────────────────────────────────────
script_dir = os.path.dirname(os.path.abspath(__file__))
if len(sys.argv) > 1:
    base = sys.argv[1]
else:
    # Walk up to find build/examples/ex_1/output
    base = os.path.join(script_dir,
                        "..", "..", "build", "examples", "ex_1", "output")
    base = os.path.normpath(base)

if not os.path.isdir(base):
    sys.exit(f"Output directory not found: {base}\nRun the simulation first.")

# Collect all runs (single run stored directly, MC runs in run0/, run1/, …)
run_dirs = sorted(glob.glob(os.path.join(base, "run*")))
if not run_dirs:
    run_dirs = [base]   # single-run layout

runs = []
for d in run_dirs:
    m_path = os.path.join(d, "missile.csv")
    t_path = os.path.join(d, "target.csv")
    if os.path.exists(m_path) and os.path.exists(t_path):
        runs.append((os.path.basename(d),
                     pd.read_csv(m_path),
                     pd.read_csv(t_path)))

if not runs:
    sys.exit("No missile.csv / target.csv found under " + base)

n_runs   = len(runs)
cmap     = cm.get_cmap("tab10")
run_cols = [cmap(i) for i in range(n_runs)]

fig = plt.figure(figsize=(16, 10))
fig.suptitle("Missile–Target Intercept (Proportional Navigation)", fontsize=14)

# ── 1. 3-D trajectory ─────────────────────────────────────────────────────────
ax3d = fig.add_subplot(2, 3, 1, projection="3d")
ax3d.set_title("3-D Trajectories")
for (label, m, t), col in zip(runs, run_cols):
    ax3d.plot(m.px, m.py, m.pz, color=col,      lw=1.5, label=f"{label} missile")
    ax3d.plot(t.px, t.py, t.pz, color=col, ls="--", lw=1, label=f"{label} target")
    # Mark intercept (last point of missile)
    ax3d.scatter(m.px.iloc[-1], m.py.iloc[-1], m.pz.iloc[-1],
                 color=col, marker="x", s=60, zorder=5)
ax3d.set_xlabel("X (m)"); ax3d.set_ylabel("Y (m)"); ax3d.set_zlabel("Z (m)")
ax3d.legend(fontsize=7, loc="upper left")

# ── 2. Top-down (X-Y) ─────────────────────────────────────────────────────────
ax_xy = fig.add_subplot(2, 3, 2)
ax_xy.set_title("Top-Down View (X–Y)")
for (label, m, t), col in zip(runs, run_cols):
    ax_xy.plot(m.px, m.py, color=col,      lw=1.5, label=f"{label} missile")
    ax_xy.plot(t.px, t.py, color=col, ls="--", lw=1, label=f"{label} target")
    ax_xy.scatter(m.px.iloc[-1], m.py.iloc[-1], color=col, marker="x", s=60, zorder=5)
ax_xy.set_xlabel("X (m)"); ax_xy.set_ylabel("Y (m)")
ax_xy.legend(fontsize=7); ax_xy.set_aspect("equal"); ax_xy.grid(True, alpha=0.3)

# ── 3. Altitude profile (X-Z) ─────────────────────────────────────────────────
ax_xz = fig.add_subplot(2, 3, 3)
ax_xz.set_title("Altitude Profile (X–Z)")
for (label, m, t), col in zip(runs, run_cols):
    ax_xz.plot(m.px, m.pz, color=col, lw=1.5, label=f"{label} missile")
    ax_xz.axhline(0, color="saddlebrown", lw=1, ls=":")  # ground
ax_xz.set_xlabel("X (m)"); ax_xz.set_ylabel("Altitude (m)")
ax_xz.legend(fontsize=7); ax_xz.grid(True, alpha=0.3)

# ── 4. Range vs time ──────────────────────────────────────────────────────────
ax_r = fig.add_subplot(2, 3, 4)
ax_r.set_title("Range to Target vs Time")
for (label, m, _), col in zip(runs, run_cols):
    ax_r.plot(m.t, m["range"], color=col, lw=1.5, label=label)
    intercept_t = m.t.iloc[-1]
    intercept_r = m["range"].iloc[-1]
    ax_r.scatter(intercept_t, intercept_r, color=col, marker="x", s=60, zorder=5)
ax_r.set_xlabel("Time (s)"); ax_r.set_ylabel("Range (m)")
ax_r.legend(fontsize=7); ax_r.grid(True, alpha=0.3)

# ── 5. Missile speed vs time ──────────────────────────────────────────────────
ax_spd = fig.add_subplot(2, 3, 5)
ax_spd.set_title("Missile Speed vs Time")
for (label, m, _), col in zip(runs, run_cols):
    speed = np.sqrt(m.vx**2 + m.vy**2 + m.vz**2)
    ax_spd.plot(m.t, speed, color=col, lw=1.5, label=label)
ax_spd.set_xlabel("Time (s)"); ax_spd.set_ylabel("Speed (m/s)")
ax_spd.legend(fontsize=7); ax_spd.grid(True, alpha=0.3)

# ── 6. Miss distance scatter (MC summary) ─────────────────────────────────────
ax_mc = fig.add_subplot(2, 3, 6)
ax_mc.set_title("Intercept Position (MC spread)")
for (label, m, t), col in zip(runs, run_cols):
    ix, iy = m.px.iloc[-1], m.py.iloc[-1]
    tx, ty = t.px.iloc[-1], t.py.iloc[-1]
    miss = np.sqrt((ix - tx)**2 + (iy - ty)**2)
    ax_mc.scatter(ix, iy, color=col, s=80, zorder=5,
                  label=f"{label}  miss={miss:.1f} m")
ax_mc.set_xlabel("X (m)"); ax_mc.set_ylabel("Y (m)")
ax_mc.legend(fontsize=8); ax_mc.grid(True, alpha=0.3)

plt.tight_layout()
out_path = os.path.join(base, "intercept_plot.png")
plt.savefig(out_path, dpi=150)
print(f"Saved: {out_path}")
plt.show()
