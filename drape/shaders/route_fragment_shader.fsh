varying float v_length;

uniform vec2 u_halfWidth;
uniform vec4 u_color;
uniform float u_clipLength;

void main(void)
{
  if (u_color.a < 0.1 || v_length < u_clipLength)
    discard;

  gl_FragColor = u_color;
}
