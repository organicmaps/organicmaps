attribute highp vec4 a_position;
attribute highp vec2 a_normal;
attribute highp vec3 a_texCoords;

uniform highp mat4 modelView;
uniform highp mat4 projection;

varying highp vec2 v_texCoords;
varying float v_textureIndex;

void main(void)
{
  gl_Position = (vec4(a_normal.xy, 0, 0) + (a_position * modelView)) * projection;
  v_texCoords = a_texCoords.st;
  v_textureIndex = a_texCoords.z;
}
