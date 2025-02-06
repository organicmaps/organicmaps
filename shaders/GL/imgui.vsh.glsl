layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoords;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec2 v_texCoords;
layout (location = 1) out vec4 v_color;

layout (binding = 0) uniform UBO
{
  mat4 u_projection;
};

void main()
{
  v_texCoords = a_texCoords;
  v_color = a_color;
  gl_Position = vec4(a_position, 0, 1) * u_projection;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
}
