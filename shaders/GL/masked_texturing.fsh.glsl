uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

varying vec2 v_colorTexCoords;
varying vec2 v_maskTexCoords;

void main()
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords) * texture2D(u_maskTex, v_maskTexCoords);
  finalColor.a *= u_opacity;
  gl_FragColor = finalColor;
}
