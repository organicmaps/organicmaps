layout (location = 0) in vec3 v_radius;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

const float kAntialiasingScalar = 0.9;

void main()
{
  float d = dot(v_radius.xy, v_radius.xy);
  vec4 finalColor = v_color;
  float aaRadius = v_radius.z * kAntialiasingScalar;
  float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z, d);
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);
  v_FragColor = finalColor;
}
