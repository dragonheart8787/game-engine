# game-engine-platform

Dual-lane integration branch that preserves both:

- **Python lane:** `pyproject.toml`, `src/game_engine`, and `pytest`.
- **C++ lane:** `CMakeLists.txt`, `engine/`, `apps/`, `tools/`, `assets/`, and `ctest`.

## CI jobs

The CI workflow runs these jobs:

- `python-test`
- `cpp-build-linux`
- `windows-sanity`
- `android-stub`

## Run checks locally

```bash
bash scripts/check-structure.sh
bash scripts/lint-docs.sh
pytest
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
bash scripts/build_android_stub.sh
```

## Ops / observability quick checks

- Dump CLI metrics snapshot:

```bash
python -m game_engine.cli --dump-metrics --format json
```

- Run multi-round benchmark helper from Python:

```python
from game_engine.ops.benchmark import run_benchmark
report = run_benchmark(lambda: None, rounds=5)
```
