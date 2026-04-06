#version 450

layout(set = 0, binding = 0) uniform sampler2D hdr_tex;

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 outColor;

vec3 tonemap_reinhard(vec3 x) {
  return x / (vec3(1.0) + x);
}

void main() {
  vec3 c = texture(hdr_tex, vUv).rgb;
  float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
  vec3 bloom = vec3(0.0);
  float rad = 0.004;
  for (int j = -2; j <= 2; ++j) {
    for (int i = -2; i <= 2; ++i) {
      vec2 o = vec2(float(i), float(j)) * rad;
      vec3 s = texture(hdr_tex, vUv + o).rgb;
      float L = dot(s, vec3(0.2126, 0.7152, 0.0722));
      if (L > 1.15)
        bloom += s * 0.038;
    }
  }
  vec3 hdr = c + bloom;
  vec3 ldr = tonemap_reinhard(hdr);
  ldr = pow(clamp(ldr, 0.0, 1.0), vec3(1.0 / 2.2));
  outColor = vec4(ldr, 1.0);
}
