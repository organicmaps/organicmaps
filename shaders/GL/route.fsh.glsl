// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

varying vec3 v_length;
varying vec4 v_color;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

uniform vec4 u_color;
uniform vec4 u_outlineColor;
uniform vec4 u_routeParams;
uniform vec4 u_maskColor;

uniform vec2 u_fakeBorders;
uniform vec4 u_fakeColor;
uniform vec4 u_fakeOutlineColor;

const float kAntialiasingThreshold = 0.92;

const float kOutlineThreshold1 = 0.81;
const float kOutlineThreshold2 = 0.71;

void main()
{
  if (v_length.x < v_length.z)
    discard;

  vec2 coefs = step(v_length.xx, u_fakeBorders);
  coefs.y = 1.0 - coefs.y;
  vec4 mainColor = mix(u_color, u_fakeColor, coefs.x);
  mainColor = mix(mainColor, u_fakeColor, coefs.y);
  vec4 mainOutlineColor = mix(u_outlineColor, u_fakeOutlineColor, coefs.x);
  mainOutlineColor = mix(mainOutlineColor, u_fakeOutlineColor, coefs.y);

  vec4 color = mix(mix(mainColor, vec4(v_color.rgb, 1.0), v_color.a), mainColor, step(u_routeParams.w, 0.0));
  color = mix(color, mainOutlineColor, step(kOutlineThreshold1, abs(v_length.y)));
  color = mix(color, mainOutlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_length.y)));
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y)));
  color = vec4(mix(color.rgb, u_maskColor.rgb, u_maskColor.a), color.a);
  gl_FragColor = samsungGoogleNexusWorkaround(color);
}
