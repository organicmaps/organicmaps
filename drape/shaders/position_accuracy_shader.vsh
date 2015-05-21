attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_accuracy;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoords;

void main(void)
{
  vec4 position = vec4(u_position, 1.0) * modelView;
  vec4 normal = vec4(normalize(a_normal) * u_accuracy, 0.0, 0.0);
  gl_Position = (position + normal) * projection;

  v_colorTexCoords = a_colorTexCoords;
}
