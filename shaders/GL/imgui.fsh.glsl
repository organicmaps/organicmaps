layout (location = 0) in vec2 v_texCoords;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  LOW_P vec4 color = texture(u_colorTex, v_texCoords);
  v_FragColor = color * v_color;
}
