import json
import subprocess
import sys
from pathlib import Path

from game_engine.ecosystem.contracts import PluginManifest, is_compatible
from game_engine.ecosystem.validators import validate_plugin_manifest
from game_engine.services.contracts import AuthTokenClaims
from game_engine.services.validators import validate_auth_claims


CLI = [sys.executable, "-m", "game_engine.cli"]


def test_compatibility_parsing() -> None:
    assert is_compatible("1.2.0", "1.0.0")
    assert not is_compatible("1.2.0", "2.0.0")
    assert not is_compatible("x", "1.0.0")


def test_validators() -> None:
    plugin_errors = validate_plugin_manifest(
        PluginManifest(
            name="",
            version="1.0.0",
            engine_version="1.0.0",
            entrypoint="",
            capabilities=[],
            dependencies=[],
            permissions=[],
        )
    )
    assert len(plugin_errors) >= 2

    auth_errors = validate_auth_claims(
        AuthTokenClaims(subject="", roles=["p"], region="", issued_at=10, expires_at=9)
    )
    assert len(auth_errors) == 3


def test_cli_validation_commands(tmp_path: Path) -> None:
    plugin = tmp_path / "plugin.json"
    plugin.write_text(
        json.dumps(
            {
                "name": "sample",
                "version": "1.0.0",
                "engine_version": "1.0.0",
                "entrypoint": "sample:main",
                "capabilities": ["runtime.system"],
                "dependencies": [],
                "permissions": [],
            }
        ),
        encoding="utf-8",
    )
    auth = tmp_path / "auth.json"
    auth.write_text(
        json.dumps(
            {
                "subject": "p",
                "roles": ["player"],
                "region": "us",
                "issued_at": 1,
                "expires_at": 2,
            }
        ),
        encoding="utf-8",
    )

    plugin_out = subprocess.run([*CLI, "validate-plugin-manifest", "--path", str(plugin)], check=True, capture_output=True, text=True)
    auth_out = subprocess.run([*CLI, "validate-auth-token", "--path", str(auth)], check=True, capture_output=True, text=True)

    assert plugin_out.stdout.strip() == "ok"
    assert auth_out.stdout.strip() == "ok"
