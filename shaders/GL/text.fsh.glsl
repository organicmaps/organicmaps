#ifdef ENABLE_VTF
varying LOW_P vec4 v_color;
#else
varying vec2 v_colorTexCoord;
uniform sampler2D u_colorTex;
#endif

varying vec2 v_maskTexCoord;

uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform vec2 u_contrastGamma;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 glyphColor = v_color;
#else
  LOW_P vec4 glyphColor = texture2D(u_colorTex, v_colorTexCoord);
#endif
#ifdef GLES3
  float dist = texture2D(u_maskTex, v_maskTexCoord).r;
#else
  float dist = texture2D(u_maskTex, v_maskTexCoord).a;
#endif
  float alpha = smoothstep(u_contrastGamma.x - u_contrastGamma.y, u_contrastGamma.x + u_contrastGamma.y, dist) * u_opacity;
  glyphColor.a *= alpha;
  gl_FragColor = glyphColor;
}
