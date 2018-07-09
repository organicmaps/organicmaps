uniform float u_opacity;

varying vec4 v_normal;
#ifdef ENABLE_VTF
varying LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoords;
#endif

const float aaPixelsCount = 2.5;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture2D(u_colorTex, v_colorTexCoords);
#endif

  float r1 = (v_normal.z - aaPixelsCount) * (v_normal.z - aaPixelsCount);
  float r2 = v_normal.x * v_normal.x + v_normal.y * v_normal.y;
  float r3 = v_normal.z * v_normal.z;
  float alpha = mix(step(r3, r2), smoothstep(r1, r3, r2), v_normal.w);

  LOW_P vec4 finalColor = color;
  finalColor.a = finalColor.a * u_opacity * (1.0 - alpha);
  if (finalColor.a == 0.0)
    discard;

  gl_FragColor = finalColor;
}
