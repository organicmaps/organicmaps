varying vec2 v_maskTexCoord;

#ifdef ENABLE_VTF
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoord;
uniform sampler2D u_colorTex;
#endif

uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform vec2 u_contrastGamma;

void main (void)
{
#ifdef ENABLE_VTF
  lowp vec4 glyphColor = v_color;
#else
  lowp vec4 glyphColor = texture2D(u_colorTex, v_colorTexCoord);
#endif
  float alpha = texture2D(u_maskTex, v_maskTexCoord).a;
  glyphColor.a *= u_opacity * alpha;
  gl_FragColor = glyphColor;
}
