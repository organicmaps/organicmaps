attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_azimut;

uniform mat4 modelView;
uniform mat4 projection;

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

  vec4 normal = vec4(a_normal, 0, 0);
  vec4 pos = vec4(u_position, 1.0);

  gl_Position = ((normal * rotation + pos * modelView)) * projection;
  v_colorTexCoords = a_colorTexCoords;
}
