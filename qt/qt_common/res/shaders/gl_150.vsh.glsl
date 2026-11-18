#version 150 core

in vec4 a_position;
out vec2 v_texCoord;

void main()
{
    v_texCoord = a_position.zw;
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
}
