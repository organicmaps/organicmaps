uniform sampler2D u_colorTex;
uniform float u_opacity;

in vec2 v_colorTexCoord;

out vec4 v_FragColor;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  v_FragColor = vec4(color.rgb, u_opacity);
}
