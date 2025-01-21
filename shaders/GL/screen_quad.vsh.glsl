in vec2 a_pos;
in vec2 a_tcoord;

out vec2 v_colorTexCoords;

void main()
{
  v_colorTexCoords = a_tcoord;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}

