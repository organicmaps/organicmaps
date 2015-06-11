varying vec2 v_length;

uniform vec4 u_color;
uniform float u_clipLength;

void main(void)
{
  if (v_length.x < u_clipLength)
    discard;

  gl_FragColor = u_color;
}
