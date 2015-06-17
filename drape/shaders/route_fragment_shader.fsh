varying vec2 v_length;

uniform vec4 u_color;
uniform float u_clipLength;

void main(void)
{
  vec4 color = u_color;
  if (v_length.x < u_clipLength)
    color.a = 0.0;

  gl_FragColor = color;
}
