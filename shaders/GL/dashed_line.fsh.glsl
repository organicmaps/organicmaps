in vec2 v_colorTexCoord;
in vec2 v_maskTexCoord;
//in vec2 v_halfLength;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

//const float aaPixelsCount = 2.5;

out vec4 v_FragColor;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  float mask = texture(u_maskTex, v_maskTexCoord).r;
  color.a = color.a * mask * u_opacity;

  // Disabled too agressive AA-like blurring of edges,
  // see https://github.com/organicmaps/organicmaps/issues/6583.
  //float currentW = abs(v_halfLength.x);
  //float diff = v_halfLength.y - currentW;
  //color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0));

  v_FragColor = color;
}
