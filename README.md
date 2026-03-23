# game-engine-platform

Integrated repository containing two lanes:

- Python lane: executable scaffold, CLI, runtime simulation, pipeline helpers, pytest suite
- C++ lane: SDL2/OpenGL ES/CMake demo engine, tools, assets, and ctest targets

## Actionable Roadmap

- Stabilize Python CLI and runtime tests
- Stabilize C++ CMake build and ctest flow
- Keep shared repository governance for both lanes
- Converge CI into dual-lane validation

## Quick start

### Python lane

```bash
pip install -e . pytest
bash scripts/check-structure.sh
bash scripts/lint-docs.sh
pytest -q
```
