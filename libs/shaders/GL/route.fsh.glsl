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
const float kOutlineThreshold1 = 0.81;
const float kOutlineThreshold2 = 0.71;

void main()
{
  if (v_length.x < v_length.z)
    discard;
  vec2 coefs = step(v_length.xx, u_fakeBorders);
  coefs.y = 1.0 - coefs.y;
  vec4 mainColor = mix(u_color, u_fakeColor, coefs.x);
  mainColor = mix(mainColor, u_fakeColor, coefs.y);
  vec4 mainOutlineColor = mix(u_outlineColor, u_fakeOutlineColor, coefs.x);
  mainOutlineColor = mix(mainOutlineColor, u_fakeOutlineColor, coefs.y);
  vec4 color = mix(mix(mainColor, vec4(v_color.rgb, 1.0), v_color.a), mainColor, step(u_routeParams.w, 0.0));
  color = mix(color, mainOutlineColor, step(kOutlineThreshold1, abs(v_length.y)));
  color = mix(color, mainOutlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_length.y)));
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y)));
  color = vec4(mix(color.rgb, u_maskColor.rgb, u_maskColor.a), color.a);
  v_FragColor = color;
}
