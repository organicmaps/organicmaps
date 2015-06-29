varying vec2 v_colorTexCoord;

uniform sampler2D u_colorTex;
uniform float u_opacity;

void main(void)
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  color.a *= u_opacity;
  gl_FragColor = color;
}
