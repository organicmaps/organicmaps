layout (location = 0) in vec3 v_normal;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_transform;
  mat4 u_normalTransform;
  vec4 u_color;
  vec2 u_texCoordFlipping;
};

const vec3 lightDir = vec3(0.316, 0.0, 0.948);

void main()
{
  float phongDiffuse = max(0.0, -dot(lightDir, v_normal));
  v_FragColor = vec4((phongDiffuse * 0.5 + 0.5) * u_color.rgb, u_color.a);
}
