// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented without discarding fragments from depth buffer.

layout (location = 0) in vec3 v_length;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_routeParams;
  vec4 u_color;
  vec4 u_maskColor;
  vec4 u_outlineColor;
  vec4 u_fakeColor;
  vec4 u_fakeOutlineColor;
  vec2 u_fakeBorders;
  vec2 u_pattern;
  vec2 u_angleCosSin;
  float u_arrowHalfWidth;
  float u_opacity;
};

const float kAntialiasingThreshold = 0.92;

float alphaFromPattern(float curLen, float dashLen, float gapLen)
{
  float len = dashLen + gapLen;
  float offset = fract(curLen / len) * len;
  return step(offset, dashLen);
}

void main()
{
  if (v_length.x < v_length.z)
    discard;
  vec2 coefs = step(v_length.xx, u_fakeBorders);
  coefs.y = 1.0 - coefs.y;
  vec4 mainColor = mix(u_color, u_fakeColor, coefs.x);
  mainColor = mix(mainColor, u_fakeColor, coefs.y);
  vec4 color = mainColor + v_color;
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))) *
              alphaFromPattern(v_length.x, u_pattern.x, u_pattern.y);
  color = vec4(mix(color.rgb, u_maskColor.rgb, u_maskColor.a), color.a);
  v_FragColor = color;
}
