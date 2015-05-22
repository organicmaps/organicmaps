varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying vec2 v_dxdy;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

void main(void)
{
  float mask = texture2D(u_maskTex, v_maskTexCoord).a;
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);

  float domen = v_dxdy.x * v_dxdy.x + v_dxdy.y * v_dxdy.y;

  if (mask < 0.1 || domen > 1.0)
    discard;

  color.a *= u_opacity;
  gl_FragColor = color;
}
