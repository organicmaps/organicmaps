varying float v_intensity;

const vec3 color = vec3(0.0, 0.75, 1.0);

void main()
{
  gl_FragColor.rgb = (v_intensity * 0.4 + 0.6) * color;
  gl_FragColor.a = 1.0;
}
