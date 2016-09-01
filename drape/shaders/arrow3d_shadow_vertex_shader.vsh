attribute vec4 a_pos;

uniform mat4 u_transform;

varying float v_intensity;

void main()
{
  vec4 position = u_transform * vec4(a_pos.x, a_pos.y, 0.0, 1.0);
  v_intensity = a_pos.w;
  gl_Position = position;
}
