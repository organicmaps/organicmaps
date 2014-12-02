attribute vec4 a_position;
attribute vec4 a_normal;
attribute vec3 a_color_index;

uniform mat4 modelView;
uniform mat4 projection;

varying vec3 v_color_index;

void main(void)
{
  gl_Position = ((a_position * modelView) + a_normal) * projection;
  v_color_index = a_color_index;
}
