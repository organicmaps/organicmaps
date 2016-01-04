varying float v_intensity;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

const vec3 color = vec3(0.0, 0.75, 1.0);

void main()
{
#ifdef SAMSUNG_GOOGLE_NEXUS
  // Because of a bug in OpenGL driver on Samsung Google Nexus this workaround is here.
  const float kFakeColorScalar = 0.0;
  lowp vec4 fakeColor = texture2D(u_colorTex, vec2(0.0, 0.0)) * kFakeColorScalar;
#endif

  vec4 resColor = vec4((v_intensity * 0.4 + 0.6) * color, 1.0);
  
#ifdef SAMSUNG_GOOGLE_NEXUS
  gl_FragColor = resColor + fakeColor;
#else
  gl_FragColor = resColor;
#endif
}
