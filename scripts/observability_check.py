#!/usr/bin/env python3
import json
import sys
from pathlib import Path

if len(sys.argv) != 3:
    print("usage: observability_check.py <schema.json> <telemetry.json>")
    sys.exit(1)

schema = json.loads(Path(sys.argv[1]).read_text())
telemetry = json.loads(Path(sys.argv[2]).read_text())
missing = [k for k in schema["required_fields"] if k not in telemetry]
if missing:
    print("missing telemetry fields:", ", ".join(missing))
    sys.exit(2)
print("observability check passed")
