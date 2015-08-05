attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;

varying vec3 v_radius;
varying vec2 v_colorTexCoords;

void main(void)
{
  gl_Position = (vec4(a_normal.xy, 0, 0) + vec4(a_position, 1) * modelView) * projection;
  v_colorTexCoords = a_colorTexCoords;
  v_radius = a_normal;
}
