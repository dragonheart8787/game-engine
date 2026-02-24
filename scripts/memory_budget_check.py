#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

if len(sys.argv) != 3:
    print("usage: memory_budget_check.py <budget.json> <assets_dir>")
    sys.exit(1)

budget = json.loads(Path(sys.argv[1]).read_text())["budgets_bytes"]
assets = Path(sys.argv[2])

usage = {}
total = 0
for root, _, files in os.walk(assets):
    for f in files:
        p = Path(root) / f
        ext = p.suffix.lower()
        size = p.stat().st_size
        usage[ext] = usage.get(ext, 0) + size
        total += size

failed = False
for ext, limit in budget.items():
    if ext == "TOTAL":
        continue
    used = usage.get(ext, 0)
    if used > limit:
        print(f"budget exceeded: {ext} used={used} limit={limit}")
        failed = True

if total > budget.get("TOTAL", 2**63-1):
    print(f"budget exceeded: TOTAL used={total} limit={budget['TOTAL']}")
    failed = True

if failed:
    sys.exit(2)
print("memory budget check passed")
