layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_normal;
layout (location = 2) in vec2 a_colorTexCoords;

layout (location = 0) out vec2 v_colorTexCoords;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  vec2 u_contrastGamma;
  vec2 u_position;
  float u_isOutlinePass;
  float u_opacity;
  float u_length;
};

void main()
{
  gl_Position = vec4(u_position + a_position + u_length * a_normal, 0, 1) * u_projection;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
  v_colorTexCoords = a_colorTexCoords;
}
