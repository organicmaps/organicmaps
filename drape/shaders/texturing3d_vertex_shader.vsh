attribute vec2 a_pos;
attribute vec2 a_tcoord;

uniform mat4 m_transform;

varying vec2 v_tcoord;

void main()
{
  v_tcoord = a_tcoord;
  gl_Position = m_transform * vec4(a_pos, 0.0, 1.0);
}

