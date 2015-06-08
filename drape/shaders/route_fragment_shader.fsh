varying vec2 v_length;

uniform vec2 u_halfWidth;
uniform vec4 u_color;
uniform float u_clipLength;

void main(void)
{
  if (u_color.a < 0.1)
    discard;

  if (v_length.x < u_clipLength)
  {
	float dist = u_clipLength - v_length.x;
	float r = dist * dist + v_length.y * v_length.y - u_halfWidth.y * u_halfWidth.y;
	if (r > 0.0)
		discard;
  }

  gl_FragColor = u_color;
}
