// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

uniform vec4 u_routeParams;
uniform vec4 u_maskColor;
uniform float u_opacity;

in vec4 v_radius;
in vec4 v_color;

const float kAntialiasingPixelsCount = 2.5;

out vec4 v_FragColor;

void main()
{
  vec4 finalColor = v_color;

  float aaRadius = max(v_radius.z - kAntialiasingPixelsCount, 0.0);
  float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z,
                               dot(v_radius.xy, v_radius.xy));
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);
  if (finalColor.a < 0.01 || u_routeParams.y > v_radius.w)
    discard;

  finalColor = vec4(mix(finalColor.rgb, u_maskColor.rgb, u_maskColor.a), finalColor.a);
  v_FragColor = finalColor;
}
