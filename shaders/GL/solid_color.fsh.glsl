uniform float u_opacity;

#ifdef ENABLE_VTF
in LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
in vec2 v_colorTexCoords;
#endif

out vec4 v_FragColor;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 finalColor = v_color;
#else
  LOW_P vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
#endif
  finalColor.a *= u_opacity;
  v_FragColor = finalColor;
}

