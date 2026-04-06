#version 450

layout(set = 0, binding = 0) uniform Ubo {
  mat4 view;
  mat4 proj;
  mat4 light_vp;
  vec4 light_dir;
  vec4 cam_pos;
} ubo;

layout(push_constant) uniform Push {
  mat4 model;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUv;

layout(location = 0) out vec3 vN;
layout(location = 1) out vec3 vW;
layout(location = 2) out vec2 vUv;
layout(location = 3) out vec4 vLP;

void main() {
  vec4 wp = pc.model * vec4(inPos, 1.0);
  vW = wp.xyz;
  mat3 nmat = mat3(pc.model);
  vN = nmat * inNorm;
  vUv = inUv;
  vLP = ubo.light_vp * wp;
  gl_Position = ubo.proj * ubo.view * wp;
}
