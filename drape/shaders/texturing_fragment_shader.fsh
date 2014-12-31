uniform sampler2D u_colorTex;

varying vec2 v_colorTexCoords;

void main(void)
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
  if (finalColor.a < 0.1)
    discard;

  gl_FragColor = finalColor;
}
