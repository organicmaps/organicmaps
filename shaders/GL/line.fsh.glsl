varying vec2 v_halfLength;

uniform float u_opacity;
#ifdef ENABLE_VTF
varying lowp vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoord;
#endif

const float aaPixelsCount = 2.5;

void main()
{
#ifdef ENABLE_VTF
  lowp vec4 color = v_color;
#else
  lowp vec4 color = texture2D(u_colorTex, v_colorTexCoord);
#endif
  color.a *= u_opacity;
  
  float currentW = abs(v_halfLength.x);
  float diff = v_halfLength.y - currentW;
  color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0));
  
  gl_FragColor = color;
}
