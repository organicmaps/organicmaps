uniform float u_opacity;

#ifdef ENABLE_VTF
varying LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoords;
#endif

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 finalColor = v_color;
#else
  LOW_P vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
#endif
  finalColor.a *= u_opacity;
  gl_FragColor = finalColor;
}

