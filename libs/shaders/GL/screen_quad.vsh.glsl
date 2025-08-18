layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_tcoord;

layout (location = 0) out vec2 v_colorTexCoords;

void main()
{
  v_colorTexCoords = a_tcoord;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}
