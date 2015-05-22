uniform sampler2D u_colorTex;
uniform float u_opacity;

varying vec2 v_colorTexCoords;

void main(void)
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
  if (finalColor.a < 0.1)
    discard;

  finalColor.a *= u_opacity;
  gl_FragColor = finalColor;
}
