#!/usr/bin/env python3
import json
import sys
from pathlib import Path

if len(sys.argv) != 3:
    print("usage: perf_regression_check.py <baseline.json> <metrics.json>")
    sys.exit(1)

baseline = json.loads(Path(sys.argv[1]).read_text())
metrics = json.loads(Path(sys.argv[2]).read_text())

tol = baseline.get("regression_tolerance_percent", 0.0) / 100.0
failed = False

for key, expected in baseline["baseline"].items():
    got = metrics.get(key)
    if got is None:
        print(f"missing metric: {key}")
        failed = True
        continue
    min_allowed = expected * (1.0 - tol)
    if got < min_allowed:
        print(f"regression: {key} expected>={min_allowed:.2f}, got={got:.2f}")
        failed = True

if failed:
    sys.exit(2)

print("perf regression check passed")
