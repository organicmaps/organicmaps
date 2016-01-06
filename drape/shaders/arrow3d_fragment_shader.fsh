varying float v_intensity;

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

  vec4 resColor = vec4((v_intensity * 0.3 + 0.7) * u_color.rgb, u_color.a);
  
#ifdef SAMSUNG_GOOGLE_NEXUS
  gl_FragColor = resColor + fakeColor;
#else
  gl_FragColor = resColor;
#endif
}
