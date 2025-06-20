// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented without discarding fragments from depth buffer.

layout (location = 0) in vec2 v_colorTexCoord;
layout (location = 1) in vec3 v_radius;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_trafficParams;
  vec4 u_outlineColor;
  vec4 u_lightArrowColor;
  vec4 u_darkArrowColor;
  float u_outline;
  float u_opacity;
};

layout (binding = 1) uniform sampler2D u_colorTex;

const float kAntialiasingThreshold = 0.92;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  float smallRadius = v_radius.z * kAntialiasingThreshold;
  float stepValue = smoothstep(smallRadius * smallRadius, v_radius.z * v_radius.z,
                               v_radius.x * v_radius.x + v_radius.y * v_radius.y);
  color.a = u_opacity * (1.0 - stepValue);
  if (color.a < 0.01)
    discard;
  v_FragColor = color;
}
