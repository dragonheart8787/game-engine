"""Validation helpers for service payloads."""

from __future__ import annotations

from .contracts import AuthTokenClaims


def validate_auth_claims(claims: AuthTokenClaims) -> list[str]:
    errors: list[str] = []
    if not claims.subject:
        errors.append("subject is required")
    if claims.expires_at <= claims.issued_at:
        errors.append("expires_at must be greater than issued_at")
    if not claims.region:
        errors.append("region is required")
    return errors
