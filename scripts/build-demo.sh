#!/usr/bin/env bash
set -euo pipefail
python -m game_engine.cli run-demo --scene examples/minimal-game/scene_boot.json --frames 3 --frame-time 16.6
