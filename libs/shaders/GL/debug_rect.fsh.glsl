layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  vec4 u_color;
};

void main()
{
  v_FragColor = u_color;
}
