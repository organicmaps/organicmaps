varying float v_intensity;

void main()
{
  gl_FragColor.rgb = (v_intensity * 0.4 + 0.6) * vec3(0.0, 0.75, 1.0);
  gl_FragColor.a = 1.0;
}
