"""Platform-level architecture contracts."""

PLATFORM_PILLARS = (
    "runtime_core",
    "editor_authoring",
    "asset_pipeline",
    "build_release_ops",
    "online_services",
    "ecosystem",
    "governance",
)

MVP_MILESTONES = {
    "M1": ["engine_loop", "scene_component_contract", "asset_guid_graph", "input_logging"],
    "M2": ["content_formats", "import_reimport_protocol", "incremental_build_manifest"],
    "M3": ["package_patch_schema", "crash_envelope", "auth_matchmaking_contracts"],
    "M4": ["plugin_lifecycle", "package_registry_metadata", "sample_plugin_game"],
}
