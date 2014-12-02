varying vec2 v_texCoords;
varying float v_textureIndex;

~getTexel~

void main(void)
{
  gl_FragColor = getTexel(int(v_textureIndex), v_texCoords);
}
