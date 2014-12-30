uniform sampler2D u_colorTex;

varying vec2 v_colorTexCoords;

void main(void)
{
  gl_FragColor = texture2D(u_colorTex, v_colorTexCoords);
}
