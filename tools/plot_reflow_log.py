#!/usr/bin/env python3
"""
Plot a reflow log from the SD card (reflows/<counter>_<profile>.csv).
Usage: python tools/plot_reflow_log.py <log.csv>
Install tools/requirements.txt before usage.
"""

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt

TEMPS = [
    "Target [C]",
    "PCB [C]",
    "Galden [C]",
    "Coolant [C]",
    "Heater 1 [C]",
    "Heater 2 [C]",
]
STATES = [
    "Heater on",
    "Lid fan",
    "Pump",
    "Quick cool fan",
    "Radiator fan",
    "Lid lock",
    "Heater OTP",
    "Coolant OTP",
]
POSITIONS = ["Lid pos", "Lift pos"]


def num(value):
    return float(value) if value.strip() else float("nan")


def read_log(path):
    meta, header, rows = {}, None, []
    with open(path, newline="", encoding="utf-8", errors="replace") as f:
        for fields in csv.reader(f, delimiter=";"):
            if not fields or fields[0].startswith("###"):
                continue
            if header is None and fields[0].startswith("Time"):
                header = [c.strip() for c in fields]
            elif header is not None and fields[0].strip().isdigit():
                rows.append(dict(zip(header, fields)))
            elif len(fields) > 1:
                meta[fields[0]] = fields[1]
    if header is None:
        raise SystemExit(f"error: no data header in {path}")
    return meta, header, rows


def plot(path):
    meta, header, rows = read_log(path)
    t = [num(r[header[0]]) for r in rows]

    def col(name):
        return [num(r.get(name, "")) for r in rows]

    fig, (ax_t, ax_s, ax_p) = plt.subplots(
        3, 1, sharex=True, figsize=(14, 10), gridspec_kw={"height_ratios": [3, 1.5, 1]}
    )

    colors, start = {}, 0
    for i in range(1, len(rows) + 1):
        if i == len(rows) or rows[i]["Phase"] != rows[start]["Phase"]:
            phase = rows[start]["Phase"]
            color = colors.setdefault(phase, f"C{len(colors)}")
            for ax in (ax_t, ax_s, ax_p):
                ax.axvspan(t[start], t[i - 1] + 1, color=color, alpha=0.08, lw=0)
            ax_t.text(
                (t[start] + t[i - 1]) / 2,
                1.01,
                phase,
                transform=ax_t.get_xaxis_transform(),
                ha="center",
                fontsize=9,
                color=color,
            )
            start = i

    for name in TEMPS:
        style = {"ls": "--", "color": "black"} if name.startswith("Target") else {}
        ax_t.plot(t, col(name), label=name, lw=1.2, **style)
    ax_t.set_ylabel("Temperature [°C]")

    for i, name in enumerate(STATES):
        ax_s.step(t, [i + (v - 0.5) * 0.6 for v in col(name)], where="post", lw=1.2)
    ax_s.set_yticks(range(len(STATES)), STATES, fontsize=8)

    for name in POSITIONS:
        ax_p.plot(t, col(name), label=name, lw=1.2)
    ax_p.set_ylabel("Position [steps]")
    ax_p.set_xlabel(header[0])

    for ax in (ax_t, ax_s, ax_p):
        ax.grid(True, alpha=0.3)
    ax_t.legend(loc="upper right", fontsize=8)
    ax_p.legend(loc="upper right", fontsize=8)
    fig.suptitle(
        f"{Path(path).name}, profile {meta.get('Profile', '?')}, "
        f"FW {meta.get('Firmware', '?')}, {meta.get('Duration', '?')} s"
    )
    fig.tight_layout()
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="Plot a reflow log.")
    parser.add_argument("csv", type=Path)
    plot(parser.parse_args().csv)


if __name__ == "__main__":
    main()
