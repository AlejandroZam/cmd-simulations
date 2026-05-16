#!/usr/bin/env python3
"""Real-time visualizer for ex_1 missile intercept simulation.

Usage (two terminals, from CMD repo root):
    Terminal 1:  python tools/viz_realtime.py [--port 9999] [--speed 1.0] [--trail 0]
    Terminal 2:  ./build/src/sim ex_1

The sim finishes in ~1-2 s (much faster than real time). The visualizer
buffers all incoming UDP packets and replays them at --speed × real time
(1.0 = wall-clock real time, 2.0 = double speed, etc.).

Press Ctrl-C or close the window to quit.
"""
import argparse
import socket
import struct
import threading
import time
from collections import defaultdict

import matplotlib
matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401  (registers 3D projection)
import numpy as np

# ── Wire protocol constants ────────────────────────────────────────────────────
PACKET_FMT  = '<BBHIdddddddd'       # must match VizBridge::Packet in viz_bridge.h
PACKET_SIZE = struct.calcsize(PACKET_FMT)  # 72 bytes

MSG_STATE     = 0
MSG_RUN_START = 1
MSG_RUN_END   = 2

ENTITY_MISSILE = 0
ENTITY_TARGET  = 1

COLORS = [
    'tab:blue', 'tab:orange', 'tab:green',
    'tab:red',  'tab:purple', 'tab:cyan',
]


# ── CLI ────────────────────────────────────────────────────────────────────────
def parse_args():
    p = argparse.ArgumentParser(description='Real-time ex_1 visualizer')
    p.add_argument('--port',  type=int,   default=9999,
                   help='UDP listen port (default 9999)')
    p.add_argument('--speed', type=float, default=1.0,
                   help='Replay speed multiplier (default 1.0 = real-time)')
    p.add_argument('--trail', type=int,   default=0,
                   help='Max trail points per entity per run (0 = unlimited)')
    return p.parse_args()


# ── Thread-safe data store ─────────────────────────────────────────────────────
class DataStore:
    """Accumulates packets from the UDP receiver thread."""

    def __init__(self):
        self._lock      = threading.Lock()
        # (run_id, entity_id) -> list of (t, px, py, pz, vx, vy, vz, range)
        self.state      = defaultdict(list)
        self.runs_seen  = set()
        self.run_ends   = set()
        self.first_wall = None   # wall time of first packet received

    def ingest(self, msg_type, entity_id, run_id, t, px, py, pz, vx, vy, vz, rng):
        with self._lock:
            self.runs_seen.add(run_id)
            if self.first_wall is None:
                self.first_wall = time.monotonic()
            if msg_type == MSG_RUN_END:
                self.run_ends.add(run_id)
            elif msg_type == MSG_STATE:
                self.state[(run_id, entity_id)].append(
                    (t, px, py, pz, vx, vy, vz, rng))

    def visible(self, display_t):
        """Return all state points with sim_t <= display_t (thread-safe copy)."""
        with self._lock:
            result = {}
            for key, pts in self.state.items():
                v = [p for p in pts if p[0] <= display_t]
                if v:
                    result[key] = v
            all_done = (len(self.run_ends) >= len(self.runs_seen)
                        and len(self.runs_seen) > 0)
            return result, all_done

    def global_bounds(self):
        """Min/max position across all buffered data (for axis limits)."""
        with self._lock:
            xs, ys, zs = [], [], []
            for pts in self.state.values():
                for p in pts:
                    xs.append(p[1]); ys.append(p[2]); zs.append(p[3])
            if not xs:
                return None
            pad = 200
            return (min(xs)-pad, max(xs)+pad,
                    min(ys)-pad, max(ys)+pad,
                    min(zs)-pad, max(zs)+pad)


# ── UDP receiver (daemon thread) ───────────────────────────────────────────────
def recv_loop(store, port, stop_ev):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', port))
    sock.settimeout(0.1)
    print(f'[viz] Listening on UDP port {port} ...')
    while not stop_ev.is_set():
        try:
            data, _ = sock.recvfrom(PACKET_SIZE)
            if len(data) != PACKET_SIZE:
                continue
            (msg_type, entity_id, run_id, _pad,
             t, px, py, pz, vx, vy, vz, rng) = struct.unpack(PACKET_FMT, data)
            store.ingest(msg_type, entity_id, run_id, t, px, py, pz, vx, vy, vz, rng)
        except socket.timeout:
            pass
        except Exception as exc:
            print(f'[viz] recv error: {exc}')
    sock.close()


# ── Visualizer ─────────────────────────────────────────────────────────────────
class Visualizer:
    def __init__(self, store, replay_speed, trail):
        self.store         = store
        self.speed         = replay_speed
        self.trail         = trail
        self.play_start    = None   # set on first update() call after data arrives
        self.display_t     = 0.0
        self._limits_set   = False

        # Artist handles: lazily created per (run_id, entity_id)
        self._trail_3d  = {}   # (run_id, entity_id) -> Line3D
        self._dot_3d    = {}   # (run_id, entity_id) -> Line3D (single point)
        self._range_2d  = {}   # run_id -> Line2D

        self._build_figure()

    def _build_figure(self):
        self.fig = plt.figure(figsize=(14, 6))
        self.fig.suptitle('Missile–Target Intercept  |  real-time replay', fontsize=13)

        self.ax3d = self.fig.add_subplot(1, 2, 1, projection='3d')
        self.ax3d.set_xlabel('X (m)')
        self.ax3d.set_ylabel('Y (m)')
        self.ax3d.set_zlabel('Z (m)')
        self.ax3d.set_title('Waiting for sim data ...')

        self.ax_r = self.fig.add_subplot(1, 2, 2)
        self.ax_r.set_title('Range to Target vs Time')
        self.ax_r.set_xlabel('Time (s)')
        self.ax_r.set_ylabel('Range (m)')
        self.ax_r.grid(True, alpha=0.3)

        plt.tight_layout()

    def _color(self, run_id):
        return COLORS[run_id % len(COLORS)]

    def _ensure_handles(self, run_id, entity_id):
        key = (run_id, entity_id)
        if key not in self._trail_3d:
            col = self._color(run_id)
            ls  = '-'       if entity_id == ENTITY_MISSILE else '--'
            mk  = 'o'       if entity_id == ENTITY_MISSILE else 's'
            tag = 'missile' if entity_id == ENTITY_MISSILE else 'target'
            line, = self.ax3d.plot([], [], [], color=col, ls=ls, lw=1.5,
                                   label=f'run{run_id} {tag}', alpha=0.85)
            dot,  = self.ax3d.plot([], [], [], mk, color=col, ms=7,
                                   markeredgecolor='k', markeredgewidth=0.5)
            self._trail_3d[key] = line
            self._dot_3d[key]   = dot
            self.ax3d.legend(fontsize=7, loc='upper left')

        if entity_id == ENTITY_MISSILE and run_id not in self._range_2d:
            col = self._color(run_id)
            rl, = self.ax_r.plot([], [], color=col, lw=1.5, label=f'run{run_id}')
            self._range_2d[run_id] = rl
            self.ax_r.legend(fontsize=7)

    def update(self, _frame):
        if self.store.first_wall is None:
            return

        # Start the replay clock on first update that has data
        if self.play_start is None:
            self.play_start = time.monotonic()

        self.display_t = (time.monotonic() - self.play_start) * self.speed
        data, all_done = self.store.visible(self.display_t)

        # Set 3D axis limits once, from global data bounds
        if not self._limits_set:
            bounds = self.store.global_bounds()
            if bounds:
                xlo, xhi, ylo, yhi, zlo, zhi = bounds
                self.ax3d.set_xlim3d(xlo, xhi)
                self.ax3d.set_ylim3d(ylo, yhi)
                self.ax3d.set_zlim3d(zlo, zhi)
                self._limits_set = True

        for (run_id, entity_id), pts in data.items():
            self._ensure_handles(run_id, entity_id)

            tail = pts[-self.trail:] if self.trail else pts
            xs = [p[1] for p in tail]
            ys = [p[2] for p in tail]
            zs = [p[3] for p in tail]

            key = (run_id, entity_id)
            self._trail_3d[key].set_data_3d(xs, ys, zs)
            self._dot_3d[key].set_data_3d([xs[-1]], [ys[-1]], [zs[-1]])

            if entity_id == ENTITY_MISSILE:
                vis_t   = [p[0]  for p in pts if p[0] <= self.display_t]
                vis_rng = [p[7]  for p in pts if p[0] <= self.display_t]
                if vis_t:
                    self._range_2d[run_id].set_data(vis_t, vis_rng)
                    self.ax_r.relim()
                    self.ax_r.autoscale_view()

        status = 'DONE' if all_done else f't ≤ {self.display_t:.1f} s'
        self.ax3d.set_title(f'3-D Trajectories  [{status}]')


# ── Entry point ────────────────────────────────────────────────────────────────
def main():
    args = parse_args()

    store   = DataStore()
    stop_ev = threading.Event()

    rx = threading.Thread(
        target=recv_loop, args=(store, args.port, stop_ev), daemon=True)
    rx.start()

    viz = Visualizer(store, args.speed, args.trail)
    ani = animation.FuncAnimation(          # noqa: F841  (must stay alive)
        viz.fig, viz.update, interval=50, cache_frame_data=False)

    print(f'[viz] speed={args.speed}×  trail={"unlimited" if not args.trail else args.trail}  '
          f'Ctrl-C or close window to quit.')
    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        stop_ev.set()


if __name__ == '__main__':
    main()
