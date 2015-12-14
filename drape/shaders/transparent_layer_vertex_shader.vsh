attribute vec2 a_pos;
attribute vec2 a_tcoord;

varying vec2 v_tcoord;

void main()
{
  v_tcoord = a_tcoord;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}

