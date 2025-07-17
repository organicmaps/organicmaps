layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_texCoords;

layout (location = 0) out float v_intensity;

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
  v_intensity = a_texCoords.x;
  gl_Position = position;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
#endif
}
