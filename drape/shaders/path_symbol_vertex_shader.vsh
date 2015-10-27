attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoords;

void main(void)
{
  lowp vec4 pos = vec4(a_position, 1) * modelView;
  highp vec4 norm = vec4(a_normal, 0, 0) * modelView;
  highp vec4 shiftedPos = norm + pos;
  gl_Position = shiftedPos * projection;
  v_colorTexCoords = a_colorTexCoords;
}
