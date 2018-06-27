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

const float kAntialiasingThreshold = 0.92;

const float kOutlineThreshold1 = 0.81;
const float kOutlineThreshold2 = 0.71;

void main()
{
  if (v_length.x < v_length.z)
    discard;

  vec4 color = mix(mix(u_color, vec4(v_color.rgb, 1.0), v_color.a), u_color, step(u_routeParams.w, 0.0));
  color = mix(color, u_outlineColor, step(kOutlineThreshold1, abs(v_length.y)));
  color = mix(color, u_outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_length.y)));
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_length.y)));
  color = vec4(mix(color.rgb, u_maskColor.rgb, u_maskColor.a), color.a);
  gl_FragColor = samsungGoogleNexusWorkaround(color);
}
