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
  gl_FragColor = getTexel(int(v_textureIndex), v_texCoords);
}
