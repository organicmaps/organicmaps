uniform float u_opacity;

in vec3 v_radius;
in vec4 v_color;

const float kAntialiasingScalar = 0.9;

out vec4 v_FragColor;

void main()
{
  float d = dot(v_radius.xy, v_radius.xy);
  vec4 finalColor = v_color;

  float aaRadius = v_radius.z * kAntialiasingScalar;
  float stepValue = smoothstep(aaRadius * aaRadius, v_radius.z * v_radius.z, d);
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);

  v_FragColor = finalColor;
}
