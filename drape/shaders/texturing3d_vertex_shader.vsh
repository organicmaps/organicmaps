attribute vec2 a_pos;
attribute vec2 a_tcoord;

uniform mat4 rotate;
uniform mat4 translate;
uniform mat4 projection;

varying vec2 v_tcoord;

void main()
{

  v_tcoord = a_tcoord;
  gl_Position = projection * translate * rotate * vec4(a_pos, 0.0, 1.0);
}

