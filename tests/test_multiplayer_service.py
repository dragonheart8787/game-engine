import asyncio
import json

from game_engine.services.multiplayer_server import start_server


async def _roundtrip(port: int) -> tuple[dict[str, object], dict[str, object]]:
    reader, writer = await asyncio.open_connection("127.0.0.1", port)
    writer.write(b'{"op":"create_lobby","lobby_id":"l1"}\n')
    await writer.drain()
    created = json.loads((await reader.readline()).decode("utf-8"))
    writer.close()
    await writer.wait_closed()

    reader2, writer2 = await asyncio.open_connection("127.0.0.1", port)
    writer2.write(b'{"op":"join_lobby","lobby_id":"l1","player_id":"p1"}\n')
    await writer2.drain()
    joined = json.loads((await reader2.readline()).decode("utf-8"))
    writer2.close()
    await writer2.wait_closed()
    return created, joined


def test_multiplayer_server_roundtrip() -> None:
    async def run() -> None:
        server = await start_server(port=8877)
        try:
            created, joined = await _roundtrip(8877)
            assert created["lobby_id"] == "l1"
            assert joined["players"] == ["p1"]
        finally:
            server.close()
            await server.wait_closed()

    asyncio.run(run())
