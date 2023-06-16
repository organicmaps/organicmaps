attribute vec3 a_pos;
attribute vec3 a_normal;

uniform mat4 u_transform;

varying vec3 v_normal;

void main()
{
  vec4 position = u_transform * vec4(a_pos, 1.0);
  v_normal = a_normal;
  gl_Position = position;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
#endif
}
