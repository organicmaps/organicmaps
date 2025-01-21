// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

in vec3 v_radius;
in vec4 v_color;

const float aaPixelsCount = 2.5;

out vec4 v_FragColor;

void main()
{
  vec4 finalColor = v_color;

  float smallRadius = v_radius.z - aaPixelsCount;
  float stepValue = smoothstep(smallRadius * smallRadius, v_radius.z * v_radius.z,
                              dot(v_radius.xy, v_radius.xy));
  finalColor.a = finalColor.a * (1.0 - stepValue);
  if (finalColor.a < 0.01)
    discard;
  v_FragColor = finalColor;
}
