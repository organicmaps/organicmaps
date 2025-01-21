uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

in vec2 v_colorTexCoords;
in vec2 v_maskTexCoords;

out vec4 v_FragColor;

void main()
{
  vec4 finalColor = texture(u_colorTex, v_colorTexCoords) * texture(u_maskTex, v_maskTexCoords);
  finalColor.a *= u_opacity;
  v_FragColor = finalColor;
}
