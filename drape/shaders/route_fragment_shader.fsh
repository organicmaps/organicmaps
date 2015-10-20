varying vec3 v_length;

uniform vec4 u_color;

void main(void)
{
  vec4 color = u_color;
  if (v_length.x < v_length.z)
    color.a = 0.0;

  gl_FragColor = color;
}
