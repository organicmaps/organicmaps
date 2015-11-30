varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform vec2 u_contrastGamma;

void main (void)
{
  vec4 glyphColor = texture2D(u_colorTex, v_colorTexCoord);
  float dist = texture2D(u_maskTex, v_maskTexCoord).a;
  float alpha = smoothstep(u_contrastGamma.x - u_contrastGamma.y, u_contrastGamma.x + u_contrastGamma.y, dist) * u_opacity;
  glyphColor.a *= alpha;
  gl_FragColor = glyphColor;
}
