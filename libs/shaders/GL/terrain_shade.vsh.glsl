layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_intensity;

layout (location = 0) out float v_intensity;

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

void main()
{
  vec4 pos = vec4(a_position, 1) * u_modelView * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  v_intensity = a_intensity;
}
