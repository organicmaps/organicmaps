layout (location = 0) in vec3 a_normal;
layout (location = 1) in vec3 a_position;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec3 v_radius;
layout (location = 1) out vec4 v_color;

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
  vec3 radius = a_normal * a_position.z;
  vec4 pos = vec4(a_position.xy, 0, 1) * u_modelView;
  vec4 shiftedPos = vec4(radius.xy, 0, 0) + pos;
  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);
  v_radius = radius;
  v_color = a_color;
}
