varying vec2 v_colorTexCoord;
varying vec2 v_halfLength;

uniform sampler2D u_colorTex;
uniform float u_opacity;

const float aaPixelsCount = 2.0;

void main(void)
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  color.a *= u_opacity;
  float currentW = abs(v_halfLength.x);
  float diff = v_halfLength.y - currentW;
  color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0));
  gl_FragColor = color;
}
