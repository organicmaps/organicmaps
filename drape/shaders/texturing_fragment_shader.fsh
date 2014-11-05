varying lowp vec2 v_texCoords;
varying lowp float v_textureIndex;

~getTexel~

void main(void)
{
  gl_FragColor = getTexel(int(v_textureIndex), v_texCoords);
}
