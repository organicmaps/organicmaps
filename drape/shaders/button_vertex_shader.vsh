attribute vec3 a_position;
attribute vec2 a_normal;

uniform mat4 modelView;
uniform mat4 projection;

void main(void)
{
  gl_Position = (vec4(a_normal, 0, 0) + vec4(a_position, 1) * modelView) * projection;
}
