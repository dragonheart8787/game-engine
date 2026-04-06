#version 450

layout(set = 0, binding = 0) uniform Ubo {
  mat4 view;
  mat4 proj;
  mat4 light_vp;
  vec4 light_dir;
  vec4 cam_pos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadow_tex;

layout(location = 0) in vec3 vN;
layout(location = 1) in vec3 vW;
layout(location = 2) in vec2 vUv;
layout(location = 3) in vec4 vLP;

layout(location = 0) out vec4 outColor;

float shadow_pcf(vec4 lp) {
  vec3 p = lp.xyz / lp.w;
  vec2 uv = p.xy * 0.5 + 0.5;
  float z = p.z * 0.5 + 0.5;
  if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
    return 1.0;
  float acc = 0.0;
  vec2 ts = 1.0 / vec2(textureSize(shadow_tex, 0));
  for (int j = -1; j <= 1; ++j) {
    for (int i = -1; i <= 1; ++i) {
      float d = texture(shadow_tex, uv + vec2(float(i), float(j)) * ts).r;
      acc += (z - 0.0012 < d) ? 1.0 : 0.0;
    }
  }
  return acc / 9.0;
}

void main() {
  vec3 N = normalize(vN);
  vec3 L = normalize(-ubo.light_dir.xyz);
  float ndl = max(dot(N, L), 0.0);
  float sh = shadow_pcf(vLP);
  vec3 base = vec3(0.55, 0.48, 0.42);
  vec3 amb = base * 0.14;
  vec3 diff = base * ndl * sh * 1.7;
  vec3 V = normalize(ubo.cam_pos.xyz - vW);
  vec3 H = normalize(L + V);
  float spec = pow(max(dot(N, H), 0.0), 40.0) * sh;
  vec3 col = amb + diff + spec * vec3(1.4, 1.35, 1.2);
  col *= 1.25;
  outColor = vec4(col, 1.0);
}
