attribute vec2 a_position;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoords;

void main(void)
{
  gl_Position = vec4(a_position, 0, 1) * modelView * projection;
  v_colorTexCoords = a_colorTexCoords;
}
