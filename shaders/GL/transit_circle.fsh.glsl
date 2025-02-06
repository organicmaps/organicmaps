// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented without discarding fragments from depth buffer.

layout (location = 0) in vec3 v_radius;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 v_FragColor;

const float aaPixelsCount = 2.5;

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
