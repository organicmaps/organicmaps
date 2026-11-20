#version 150 core

out vec2 v_texCoord;

void main()
{
    v_texCoord = vec2(gl_VertexID % 2, 1 - gl_VertexID / 2);
    gl_Position = vec4(v_texCoord * 2.0 - 1.0, 0.0, 1.0);
}
