attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_azimut;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

varying vec2 v_colorTexCoords;

void main()
{
  float sinV = sin(u_azimut);
  float cosV = cos(u_azimut);

  mat4 rotation;
  rotation[0] = vec4(cosV, sinV, 0.0, 0.0);
  rotation[1] = vec4(-sinV, cosV, 0.0, 0.0);
  rotation[2] = vec4(0.0, 0.0, 1.0, 0.0);
  rotation[3] = vec4(0.0, 0.0, 0.0, 1.0);

  vec4 pos = vec4(u_position, 1.0) * u_modelView;
  vec4 normal = vec4(a_normal, 0, 0);
  vec4 shiftedPos = normal * rotation + pos;

  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);
  v_colorTexCoords = a_colorTexCoords;
}
