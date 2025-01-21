in vec2 v_texCoords;
in vec4 v_color;

uniform sampler2D u_colorTex;

out vec4 v_FragColor;

void main()
{
  LOW_P vec4 color = texture(u_colorTex, v_texCoords);
  v_FragColor = color * v_color;
}
