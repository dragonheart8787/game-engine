"""Editor command model and undo/redo history."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Protocol


class EditorCommand(Protocol):
    name: str

    def execute(self) -> None: ...

    def undo(self) -> None: ...


@dataclass(slots=True)
class EditorHistory:
    _undo_stack: list[EditorCommand] = field(default_factory=list)
    _redo_stack: list[EditorCommand] = field(default_factory=list)

    def execute(self, command: EditorCommand) -> None:
        command.execute()
        self._undo_stack.append(command)
        self._redo_stack.clear()

    def undo(self) -> str | None:
        if not self._undo_stack:
            return None
        command = self._undo_stack.pop()
        command.undo()
        self._redo_stack.append(command)
        return command.name

    def redo(self) -> str | None:
        if not self._redo_stack:
            return None
        command = self._redo_stack.pop()
        command.execute()
        self._undo_stack.append(command)
        return command.name

    @property
    def can_undo(self) -> bool:
        return bool(self._undo_stack)

    @property
    def can_redo(self) -> bool:
        return bool(self._redo_stack)
