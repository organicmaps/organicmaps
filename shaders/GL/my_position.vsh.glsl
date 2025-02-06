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
  float sinV = sin(u_azimut);
  float cosV = cos(u_azimut);
  mat4 rotation;
  rotation[0] = vec4(cosV, sinV, 0.0, 0.0);
  rotation[1] = vec4(-sinV, cosV, 0.0, 0.0);
  rotation[2] = vec4(0.0, 0.0, 1.0, 0.0);
  rotation[3] = vec4(0.0, 0.0, 0.0, 1.0);
  vec4 pos = vec4(u_position.xyz, 1.0) * u_modelView;
  vec4 normal = vec4(a_normal, 0, 0);
  vec4 shiftedPos = normal * rotation + pos;
  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);
  v_colorTexCoords = a_colorTexCoords;
}
