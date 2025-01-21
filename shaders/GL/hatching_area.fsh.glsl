uniform float u_opacity;

#ifdef ENABLE_VTF
in LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
in vec2 v_colorTexCoords;
#endif

uniform sampler2D u_maskTex;
in vec2 v_maskTexCoords;

out vec4 v_FragColor;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif
  color *= texture(u_maskTex, v_maskTexCoords);
  color.a *= u_opacity;
  v_FragColor = color;
}

