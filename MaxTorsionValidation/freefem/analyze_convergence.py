#!/usr/bin/env python3
"""Estimate a dyadic convergence order from FreeFEM MODE output."""

from __future__ import annotations

import math
import re
import subprocess
import sys


MODE = re.compile(
    r"MODE k=(\d+) lambda_minus=([-+\deE.]+) lambda_plus=([-+\deE.]+)"
)


def run(ff: str, script: str, n: int, m: int) -> dict[tuple[int, str], float]:
    text = subprocess.check_output(
        [ff, "-nw", script, "-n", str(n), "-m", str(m)], text=True
    )
    values: dict[tuple[int, str], float] = {}
    for match in MODE.finditer(text):
        k = int(match.group(1))
        values[k, "-"] = float(match.group(2))
        values[k, "+"] = float(match.group(3))
    return values


def main() -> None:
    if len(sys.argv) not in (4, 5):
        raise SystemExit("usage: analyze_convergence.py FREEFEM SCRIPT n [m0=8]")
    ff, script, n = sys.argv[1], sys.argv[2], int(sys.argv[3])
    m0 = int(sys.argv[4]) if len(sys.argv) == 5 else 8
    samples = [run(ff, script, n, m) for m in (m0, 2 * m0, 4 * m0)]
    print(f"n={n}; meshes m={m0},{2*m0},{4*m0}")
    print("mode branch value(finest) observed_order")
    for key in sorted(samples[0]):
        v0, v1, v2 = (sample[key] for sample in samples)
        den = v1 - v2
        ratio = (v0 - v1) / den if den else math.nan
        order = math.log(abs(ratio), 2) if ratio and math.isfinite(ratio) else math.nan
        print(f"{key[0]:4d} {key[1]:>6s} {v2: .12e} {order: .6f}")


if __name__ == "__main__":
    main()

