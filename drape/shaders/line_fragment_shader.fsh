varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

void main(void)
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  color.a *= (u_opacity * texture2D(u_maskTex, v_maskTexCoord).a);
  gl_FragColor = color;
}
