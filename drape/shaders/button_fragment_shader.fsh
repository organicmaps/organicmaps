uniform vec4 u_color;
uniform float u_opacity;

void main(void)
{
  vec4 color = u_color;
  if (color.a < 0.1)
    discard;

  color.a *= u_opacity;
  gl_FragColor = color;
}
