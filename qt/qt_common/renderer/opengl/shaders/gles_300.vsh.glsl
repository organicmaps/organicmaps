#version 300 es

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

in vec4 a_position;
uniform vec2 u_samplerSize;
out vec2 v_texCoord;

void main()
{
    v_texCoord = vec2(a_position.z * u_samplerSize.x, a_position.w * u_samplerSize.y);
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
}
