uniform float u_opacity;

#ifdef ENABLE_VTF
varying lowp vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoords;
#endif

void main()
{
#ifdef ENABLE_VTF
  lowp vec4 finalColor = v_color;
#else
  lowp vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
#endif
  finalColor.a *= u_opacity;
  gl_FragColor = finalColor;
}

