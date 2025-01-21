uniform float u_opacity;
#ifdef ENABLE_VTF
in LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
in vec2 v_colorTexCoord;
#endif

//in vec2 v_halfLength;

//const float aaPixelsCount = 2.5;

out vec4 v_FragColor;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoord);
#endif
  color.a *= u_opacity;

  // Disabled too agressive AA-like blurring of edges,
  // see https://github.com/organicmaps/organicmaps/issues/6583.
  //float currentW = abs(v_halfLength.x);
  //float diff = v_halfLength.y - currentW;
  //color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0));

  v_FragColor = color;
}
