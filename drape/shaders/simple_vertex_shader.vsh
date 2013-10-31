attribute mediump vec2 position;
attribute mediump float depth;

uniform mediump mat4 modelViewProjectionMatrix;

void main(void)
{
    gl_Position = modelViewProjectionMatrix * vec4(position, depth, 1.0);
}
