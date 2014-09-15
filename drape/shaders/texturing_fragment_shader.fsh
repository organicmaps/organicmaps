#ifdef GL_FRAGMENT_PRECISION_HIGH
  #define MAXPREC highp
#else
  #define MAXPREC mediump
#endif
varying lowp vec2 v_texCoords;
varying MAXPREC float v_textureIndex;

~getTexel~

void main(void)
{
  int textureIndex = int(v_textureIndex);
  gl_FragColor = getTexel(textureIndex, v_texCoords);
}
