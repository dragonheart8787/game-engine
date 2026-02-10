# third_party

Offline vendor mode (`-DENGINE_VENDOR_DEPS=ON`) expects dependencies to be available from either:
1. System packages (`glm`, `nlohmann_json`), or
2. Checked-in folders:
   - `third_party/glm`
   - `third_party/json`

Pinned versions:
- glm: 0.9.9.8
- nlohmann/json: v3.11.3

Use scripts/fetch_deps.sh to prefetch vendored sources.
