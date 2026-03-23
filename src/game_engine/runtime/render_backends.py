"""Pluggable render backends.

Note: This module provides executable backend adapters with per-backend command
encoders; it is still a software simulation layer in Python, not native GPU API bindings.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class RenderBackend(str, Enum):
    VULKAN = "vulkan"
    DIRECTX12 = "dx12"
    METAL = "metal"


@dataclass(slots=True)
class RenderFrame:
    backend: RenderBackend
    frame_index: int
    draw_calls: int
    command_buffer: list[str]


class BaseBackend:
    backend: RenderBackend

    def encode_frame(self, frame_index: int, draw_calls: int) -> RenderFrame:
        raise NotImplementedError


class VulkanBackend(BaseBackend):
    backend = RenderBackend.VULKAN

    def encode_frame(self, frame_index: int, draw_calls: int) -> RenderFrame:
        cmds = ["vkBeginCommandBuffer", f"vkCmdDraw x{draw_calls}", "vkEndCommandBuffer"]
        return RenderFrame(self.backend, frame_index, draw_calls, cmds)


class Dx12Backend(BaseBackend):
    backend = RenderBackend.DIRECTX12

    def encode_frame(self, frame_index: int, draw_calls: int) -> RenderFrame:
        cmds = ["ID3D12GraphicsCommandList::Reset", f"DrawInstanced x{draw_calls}", "Close"]
        return RenderFrame(self.backend, frame_index, draw_calls, cmds)


class MetalBackend(BaseBackend):
    backend = RenderBackend.METAL

    def encode_frame(self, frame_index: int, draw_calls: int) -> RenderFrame:
        cmds = ["MTLCommandBuffer.begin", f"drawPrimitives x{draw_calls}", "commit"]
        return RenderFrame(self.backend, frame_index, draw_calls, cmds)


def create_backend(backend: RenderBackend) -> BaseBackend:
    if backend is RenderBackend.VULKAN:
        return VulkanBackend()
    if backend is RenderBackend.DIRECTX12:
        return Dx12Backend()
    if backend is RenderBackend.METAL:
        return MetalBackend()
    raise ValueError(f"Unsupported backend: {backend}")
