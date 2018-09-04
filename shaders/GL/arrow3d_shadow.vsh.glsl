attribute vec4 a_pos;

// The same geometry can be rendered with different shader, where a_normal attribute is used.
// On some platforms unused attributes can cause an incorrect offset calculation, so we add
// fake usage of this attribute.
attribute vec3 a_normal;

uniform mat4 u_transform;

varying float v_intensity;

void main()
{
  vec4 position = u_transform * vec4(a_pos.x, a_pos.y, 0.0, 1.0);
  v_intensity = a_pos.w;
  gl_Position = position + vec4(a_normal - a_normal, 0.0);
}
