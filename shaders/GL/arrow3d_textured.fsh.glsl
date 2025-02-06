layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_texCoords;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_transform;
  mat4 u_normalTransform;
  vec4 u_color;
  vec2 u_texCoordFlipping;
};

layout (binding = 1) uniform sampler2D u_colorTex;

const vec3 lightDir = vec3(0.316, 0.0, 0.948);

void main()
{
  float phongDiffuse = max(0.0, -dot(lightDir, v_normal));
  vec4 color = texture(u_colorTex, v_texCoords) * u_color;
  v_FragColor = vec4((phongDiffuse * 0.5 + 0.5) * color.rgb, color.a);
}
