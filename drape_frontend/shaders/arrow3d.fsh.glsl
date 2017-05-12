varying vec2 v_intensity;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

uniform vec4 u_color;

void main()
{
  float alpha = smoothstep(0.8, 1.0, v_intensity.y);
  vec4 resColor = vec4((v_intensity.x * 0.5 + 0.5) * u_color.rgb, u_color.a * alpha);
  gl_FragColor = samsungGoogleNexusWorkaround(resColor);
}
