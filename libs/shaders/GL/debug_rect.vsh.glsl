layout (location = 0) in vec2 a_position;

void main()
{
  gl_Position = vec4(a_position, 0, 1);
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
}
