"""Async multiplayer lobby/matchmaking service."""

from __future__ import annotations

import asyncio
import json
from dataclasses import dataclass, field


@dataclass(slots=True)
class Lobby:
    lobby_id: str
    players: list[str] = field(default_factory=list)


class MatchmakingService:
    def __init__(self) -> None:
        self.lobbies: dict[str, Lobby] = {}

    def create_lobby(self, lobby_id: str) -> Lobby:
        lobby = Lobby(lobby_id=lobby_id)
        self.lobbies[lobby_id] = lobby
        return lobby

    def join_lobby(self, lobby_id: str, player_id: str) -> Lobby:
        lobby = self.lobbies[lobby_id]
        if player_id not in lobby.players:
            lobby.players.append(player_id)
        return lobby


async def start_server(host: str = "127.0.0.1", port: int = 8765) -> asyncio.base_events.Server:
    service = MatchmakingService()

    async def handle(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        data = await reader.readline()
        msg = json.loads(data.decode("utf-8"))
        op = msg.get("op")
        if op == "create_lobby":
            lobby = service.create_lobby(msg["lobby_id"])
            writer.write((json.dumps({"lobby_id": lobby.lobby_id, "players": lobby.players}) + "\n").encode("utf-8"))
        elif op == "join_lobby":
            lobby = service.join_lobby(msg["lobby_id"], msg["player_id"])
            writer.write((json.dumps({"lobby_id": lobby.lobby_id, "players": lobby.players}) + "\n").encode("utf-8"))
        else:
            writer.write(b'{"error":"unknown op"}\n')
        await writer.drain()
        writer.close()
        await writer.wait_closed()

    return await asyncio.start_server(handle, host, port)
