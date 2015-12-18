attribute vec2 a_normal;

uniform mat4 modelView;
uniform mat4 projection;

void main(void)
{
  gl_Position = vec4(modelView[0][3] + a_normal.x, modelView[1][3] + a_normal.y, modelView[2][3], 1) * projection;
}
