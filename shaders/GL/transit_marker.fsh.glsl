#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

varying vec4 v_offsets;
varying vec4 v_color;
varying float v_scale;

void main()
{
  vec4 finalColor = v_color;
  vec2 radius;
  radius.x = max(0.0, abs(v_offsets.x) - v_offsets.z);
  radius.y = max(0.0, abs(v_offsets.y) - v_offsets.w);

  float maxRadius = 1.0;
  float aaRadius = 0.9;
  float stepValue = smoothstep(aaRadius * aaRadius, maxRadius * maxRadius, dot(radius.xy, radius.xy));
  finalColor.a = finalColor.a * (1.0 - stepValue);

  gl_FragColor = samsungGoogleNexusWorkaround(finalColor);
}
