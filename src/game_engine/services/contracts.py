"""Online service contracts."""

from dataclasses import dataclass


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
