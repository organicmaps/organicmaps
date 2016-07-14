varying vec2 v_intensity;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

uniform vec4 u_color;

void main()
{
#ifdef SAMSUNG_GOOGLE_NEXUS
  // Because of a bug in OpenGL driver on Samsung Google Nexus this workaround is here.
  const float kFakeColorScalar = 0.0;
  lowp vec4 fakeColor = texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar;
#endif

  float alpha = smoothstep(0.8, 1.0, v_intensity.y);
  vec4 resColor = vec4((v_intensity.x * 0.5 + 0.5) * u_color.rgb, u_color.a * alpha);
  
#ifdef SAMSUNG_GOOGLE_NEXUS
  gl_FragColor = resColor + fakeColor;
#else
  gl_FragColor = resColor;
#endif
}
