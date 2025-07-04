// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented without discarding fragments from depth buffer.

layout (location = 0) in vec4 v_radius;
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

const float kAntialiasingPixelsCount = 2.5;

void main()
{
  vec4 finalColor = v_color;
  float aaRadius = max(v_radius.z - kAntialiasingPixelsCount, 0.0);
  float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z,
                               dot(v_radius.xy, v_radius.xy));
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);
  if (finalColor.a < 0.01 || u_routeParams.y > v_radius.w)
    discard;
  finalColor = vec4(mix(finalColor.rgb, u_maskColor.rgb, u_maskColor.a), finalColor.a);
  v_FragColor = finalColor;
}
