#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 0) out vec4 out_color;

void main() {
  // Phase1：簡化光照（固定法線 + 定向光）+ Reinhard tone + gamma（bloom／shadow map 待接）。
  vec3 N = normalize(vec3(0.0, 0.0, 1.0));
  vec3 L = normalize(vec3(0.35, -0.72, 0.28));
  float ndl = max(dot(N, L), 0.0);
  vec3 albedo = frag_color;
  vec3 ambient = 0.06 * albedo;
  vec3 lit = ambient + ndl * albedo * 0.94;
  float shadow_hint = 0.78 + 0.22 * ndl;
  lit *= shadow_hint;
  vec3 mapped = lit / (lit + vec3(1.0));
  mapped = pow(mapped, vec3(1.0 / 2.2));
  out_color = vec4(mapped, 1.0);
}
