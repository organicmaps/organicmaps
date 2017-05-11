varying vec3 v_length;
varying vec4 v_color;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

uniform vec4 u_color;
uniform vec2 u_pattern;

const float kAntialiasingThreshold = 0.92;

float alphaFromPattern(float curLen, float dashLen, float gapLen)
{
  float len = dashLen + gapLen;
  float offset = fract(curLen / len) * len;
  return step(offset, dashLen);
}

void main()
{
  vec4 color = u_color;
  if (v_length.x < v_length.z)
  {
    color.a = 0.0;
  }
  else
  {
    color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))) *
               alphaFromPattern(v_length.x, u_pattern.x, u_pattern.y);
  }
  gl_FragColor = samsungGoogleNexusWorkaround(color);
}
