attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec3 a_texCoords;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_texCoords;
varying float v_textureIndex;

void main(void)
{
  gl_Position = (vec4(a_normal.xy, 0, 0) + a_position * modelView) * projection;
  v_texCoords = a_texCoords.st;
  v_textureIndex = a_texCoords.z;
}
