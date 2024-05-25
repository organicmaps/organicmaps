#version 300 es

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform sampler2D u_sampler;
in vec2 v_texCoord;
out vec4 v_FragColor;

void main()
{
    v_FragColor = vec4(texture(u_sampler, v_texCoord).rgb, 1.0);
}
