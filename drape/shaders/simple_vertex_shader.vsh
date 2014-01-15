attribute vec4 position;

uniform mat4 modelView;
uniform mat4 projection;

void main(void)
{
    gl_Position = position * modelView * projection;
}
