in float v_intensity;

uniform vec4 u_color;

out vec4 v_FragColor;

void main()
{
  v_FragColor = vec4(u_color.rgb, u_color.a * v_intensity);
}
