layout (location = 0) in vec2 a_normal;
layout (location = 1) in vec2 a_colorTexCoords;

layout (location = 0) out vec2 v_colorTexCoords;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_position;
  vec2 u_lineParams;
  float u_accuracy;
  float u_zScale;
  float u_opacity;
  float u_azimut;
};

void main()
{
  vec4 position = vec4(u_position.xy, 0.0, 1.0) * u_modelView;
  vec4 normal = vec4(a_normal * u_accuracy, 0.0, 0.0);
  position = (position + normal) * u_projection;
  gl_Position = applyPivotTransform(position, u_pivotTransform, u_position.z * u_zScale);
  v_colorTexCoords = a_colorTexCoords;
}
