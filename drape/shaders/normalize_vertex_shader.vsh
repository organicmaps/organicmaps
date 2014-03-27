attribute vec4 a_position;
attribute vec4 a_normal;

uniform mat4 modelView;
uniform mat4 projection;

void main(void)
{
  gl_Position = ((a_position * modelView) + a_normal) * projection;
}
