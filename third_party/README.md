# third_party

Offline vendor mode (`-DENGINE_VENDOR_DEPS=ON`) requires dependencies from either:
1. System packages (`glm`, `nlohmann_json`), or
2. Vendored folders:
   - `third_party/glm`
   - `third_party/json`

Pinned versions:
- glm: 0.9.9.8
- nlohmann/json: v3.11.3

## Initialize vendored deps

### Option A: git submodule
```bash
git submodule update --init --recursive
```

### Option B: fetch script
```bash
./scripts/fetch_deps.sh --vendor-dir third_party --glm 0.9.9.8 --json v3.11.3
```

Windows PowerShell:
```powershell
./scripts/fetch_deps.ps1 -VendorDir third_party -Glm 0.9.9.8 -Json v3.11.3
```
