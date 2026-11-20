#version 300 es

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

out vec2 v_texCoord;

void main()
{
    v_texCoord = vec2(gl_VertexID % 2, 1 - gl_VertexID / 2);
    gl_Position = vec4(v_texCoord * 2.0 - 1.0, 0.0, 1.0);
}
