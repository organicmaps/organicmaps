layout (location = 0) in vec2 v_colorTexCoords;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  float u_opacity;
};

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
  finalColor.a *= u_opacity;
  v_FragColor = finalColor;
}
