attribute vec2 position;
attribute float depth;

uniform mat4 modelView;
uniform mat4 projection;

void main(void)
{
    gl_Position = vec4(position, depth, 1.0) * modelView * projection;
}
