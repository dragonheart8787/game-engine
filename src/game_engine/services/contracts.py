"""Online service contracts."""

from dataclasses import dataclass
from typing import Any


@dataclass(slots=True)
class AuthTokenClaims:
    subject: str
    roles: list[str]
    region: str
    issued_at: int
    expires_at: int


@dataclass(slots=True)
class LobbyTicket:
    lobby_id: str
    player_id: str
    queue_name: str


@dataclass(slots=True)
class ReplicationSnapshot:
    node_id: str
    state: dict[str, Any]


@dataclass(slots=True)
class ServiceHealth:
    service_name: str
    status: str
    details: str = ""
