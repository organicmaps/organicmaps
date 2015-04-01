attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_accuracy;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoords;

void main(void)
{
  vec4 position = vec4(u_position.xy + normalize(a_normal) * u_accuracy, u_position.z, 1);
  gl_Position = position * modelView * projection;

  v_colorTexCoords = a_colorTexCoords;
}
