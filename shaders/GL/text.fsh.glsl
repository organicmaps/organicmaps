#ifdef ENABLE_VTF
in LOW_P vec4 v_color;
#else
in vec2 v_colorTexCoord;
uniform sampler2D u_colorTex;
#endif

in vec2 v_maskTexCoord;

uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform vec2 u_contrastGamma;

out vec4 v_FragColor;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 glyphColor = v_color;
#else
  LOW_P vec4 glyphColor = texture(u_colorTex, v_colorTexCoord);
#endif
  float dist = texture(u_maskTex, v_maskTexCoord).r;
  float alpha = smoothstep(u_contrastGamma.x - u_contrastGamma.y, u_contrastGamma.x + u_contrastGamma.y, dist) * u_opacity;
  glyphColor.a *= alpha;
  v_FragColor = glyphColor;
}
