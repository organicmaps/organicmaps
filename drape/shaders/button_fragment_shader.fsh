uniform vec4 u_color;

void main(void)
{
  if (u_color.a < 0.1)
    discard;

  gl_FragColor = u_color;
}
