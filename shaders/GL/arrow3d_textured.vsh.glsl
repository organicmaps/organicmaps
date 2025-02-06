layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texCoords;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_texCoords;

layout (binding = 0) uniform UBO
{
  mat4 u_transform;
  mat4 u_normalTransform;
  vec4 u_color;
  vec2 u_texCoordFlipping;
};

void main()
{
  vec4 position = u_transform * vec4(a_pos, 1.0);
  v_normal = normalize((u_normalTransform * vec4(a_normal, 0.0)).xyz);
  v_texCoords = mix(a_texCoords, 1.0 - a_texCoords, u_texCoordFlipping);
  gl_Position = position;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
#endif
}
