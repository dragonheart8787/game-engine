#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUv;

layout(push_constant) uniform Push {
  mat4 mvp;
} pc;

void main() {
  gl_Position = pc.mvp * vec4(inPos, 1.0);
}
