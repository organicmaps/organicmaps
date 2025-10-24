
layout (binding = 1) uniform sampler2D u_colorTex;

layout (location = 0) in vec3 v_texCoords;

layout (location = 0) out vec4 v_FragColor;

void main()
{
  v_FragColor = texture(u_colorTex, v_texCoords.xy);
}
