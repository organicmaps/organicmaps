uniform float u_opacity;

#ifdef ENABLE_VTF
varying LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoords;
#endif

uniform sampler2D u_maskTex;
varying vec2 v_maskTexCoords;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture2D(u_colorTex, v_colorTexCoords);
#endif
  color *= texture2D(u_maskTex, v_maskTexCoords);
  color.a *= u_opacity;
  gl_FragColor = color;
}

