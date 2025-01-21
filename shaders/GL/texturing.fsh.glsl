uniform sampler2D u_colorTex;
uniform float u_opacity;

in vec2 v_colorTexCoords;

out vec4 v_FragColor;

void main()
{
  vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
  finalColor.a *= u_opacity;
  v_FragColor = finalColor;
}
