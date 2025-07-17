// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

varying vec3 v_length;
varying vec4 v_color;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

uniform vec4 u_color;
uniform vec2 u_pattern;
uniform vec4 u_maskColor;

uniform vec2 u_fakeBorders;
uniform vec4 u_fakeColor;

const float kAntialiasingThreshold = 0.92;

float alphaFromPattern(float curLen, float dashLen, float gapLen)
{
  float len = dashLen + gapLen;
  float offset = fract(curLen / len) * len;
  return step(offset, dashLen);
}

void main()
{
  if (v_length.x < v_length.z)
    discard;

  vec2 coefs = step(v_length.xx, u_fakeBorders);
  coefs.y = 1.0 - coefs.y;
  vec4 mainColor = mix(u_color, u_fakeColor, coefs.x);
  mainColor = mix(mainColor, u_fakeColor, coefs.y);

  vec4 color = mainColor + v_color;
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y))) *
              alphaFromPattern(v_length.x, u_pattern.x, u_pattern.y);
  color = vec4(mix(color.rgb, u_maskColor.rgb, u_maskColor.a), color.a);
  gl_FragColor = samsungGoogleNexusWorkaround(color);
}
