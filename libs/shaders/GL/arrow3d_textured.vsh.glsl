attribute vec3 a_pos;
attribute vec3 a_normal;
attribute vec2 a_texCoords;

uniform mat4 u_transform;
uniform mat4 u_normalTransform;
uniform vec2 u_texCoordFlipping;

varying vec3 v_normal;
varying vec2 v_texCoords;

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
