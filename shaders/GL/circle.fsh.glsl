uniform float u_opacity;
#ifdef ENABLE_VTF
in LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
in vec2 v_colorTexCoords;
#endif

in vec3 v_radius;

const float aaPixelsCount = 2.5;

out vec4 v_FragColor;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 finalColor = v_color;
#else
  LOW_P vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
#endif

  float smallRadius = v_radius.z - aaPixelsCount;
  float stepValue = smoothstep(smallRadius * smallRadius, v_radius.z * v_radius.z,
                               v_radius.x * v_radius.x + v_radius.y * v_radius.y);
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);
  v_FragColor = finalColor;
}
