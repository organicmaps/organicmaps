#ifdef SAMSUNG_GOOGLE_NEXUS
varying lowp vec4 v_fakeColor;
#endif

uniform float u_opacity;
uniform float u_radiusShift;

varying vec3 v_radius;
varying vec4 v_color;

const float kAntialiasingScalar = 0.9;
const vec3 kOutlineColor = vec3(1.0, 1.0, 1.0);

void main(void)
{
#ifdef SAMSUNG_GOOGLE_NEXUS
  // Because of a bug in OpenGL driver on Samsung Google Nexus this workaround is here.
  const float kFakeColorScalar = 0.0;
  lowp vec4 fakeColor = v_fakeColor * kFakeColorScalar;
#endif

  float d = dot(v_radius.xy, v_radius.xy);
  
  float shiftedRadius = v_radius.z - u_radiusShift;
  float aaRadius = shiftedRadius * kAntialiasingScalar;
  vec4 finalColor = vec4(mix(v_color.rgb, kOutlineColor, smoothstep(aaRadius * aaRadius, shiftedRadius * shiftedRadius, d)), v_color.a);
  
  aaRadius = v_radius.z * kAntialiasingScalar;
  float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z, d);
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);

#ifdef SAMSUNG_GOOGLE_NEXUS
  gl_FragColor = finalColor + fakeColor;
#else
  gl_FragColor = finalColor;
#endif
}
