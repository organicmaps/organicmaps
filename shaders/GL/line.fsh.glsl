uniform float u_opacity;
#ifdef ENABLE_VTF
varying LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoord;
#endif

varying vec2 v_halfLength;

const float aaPixelsCount = 2.5;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture2D(u_colorTex, v_colorTexCoord);
#endif
  color.a *= u_opacity;
  
  float currentW = abs(v_halfLength.x);
  float diff = v_halfLength.y - currentW;
  color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0));
  
  gl_FragColor = color;
}
