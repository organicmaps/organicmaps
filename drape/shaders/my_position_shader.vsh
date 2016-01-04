attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_azimut;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoords;

void main(void)
{
  float sinV = sin(u_azimut);
  float cosV = cos(u_azimut);

  mat4 rotation;
  rotation[0] = vec4(cosV, sinV, 0.0, 0.0);
  rotation[1] = vec4(-sinV, cosV, 0.0, 0.0);
  rotation[2] = vec4(0.0, 0.0, 1.0, 0.0);
  rotation[3] = vec4(0.0, 0.0, 0.0, 1.0);

  lowp vec4 pos = vec4(u_position, 1.0) * modelView;
  highp vec4 normal = vec4(a_normal, 0, 0);
  highp vec4 shiftedPos = normal * rotation + pos;

  shiftedPos = shiftedPos * projection;
  float w = shiftedPos.w;
  shiftedPos.xyw = (pivotTransform * vec4(shiftedPos.xy, 0.0, w)).xyw;
  shiftedPos.z *= shiftedPos.w / w;
  gl_Position = shiftedPos;
  v_colorTexCoords = a_colorTexCoords;
}
