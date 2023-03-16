attribute vec4 a_pos;
attribute vec3 a_normal;

uniform mat4 u_transform;

varying vec2 v_intensity;

const vec3 lightDir = vec3(0.316, 0.0, 0.948);

void main()
{
  vec4 position = u_transform * vec4(a_pos.xyz, 1.0);
  v_intensity = vec2(max(0.0, -dot(lightDir, a_normal)), a_pos.w);
  gl_Position = position;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
}
