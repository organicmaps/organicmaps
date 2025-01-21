in vec3 a_pos;
in vec2 a_texCoords;

uniform mat4 u_transform;

out float v_intensity;

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
