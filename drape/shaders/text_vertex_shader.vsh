attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec2 a_outlineColorTexCoord;
attribute vec2 a_maskTexCoord;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying vec2 v_outlineColorTexCoord;

void main()
{
  gl_Position = (vec4(a_normal, 0, 0) + a_position * modelView) * projection;
  v_colorTexCoord = a_colorTexCoord;
  v_maskTexCoord = a_maskTexCoord;
  v_outlineColorTexCoord = a_outlineColorTexCoord;
}
