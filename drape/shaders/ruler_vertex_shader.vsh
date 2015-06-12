attribute vec2 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec2 u_position;
uniform float u_length;
uniform mat4 projection;

varying vec2 v_colorTexCoords;

void main(void)
{
  gl_Position = vec4(u_position + a_position + u_length * a_normal, 0, 1) * projection;
  v_colorTexCoords = a_colorTexCoords;
}
