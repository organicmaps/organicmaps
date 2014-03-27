attribute highp vec4 a_position;
attribute highp vec4 a_normal;
attribute highp vec4 a_texCoords;

uniform highp mat4 modelView;
uniform highp mat4 projection;

varying highp vec4 v_texCoords;

void main(void)
{
  gl_Position = ((a_position * modelView) + a_normal) * projection;
  v_texCoords = a_texCoords;
}
