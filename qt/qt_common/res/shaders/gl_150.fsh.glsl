#version 150 core

uniform sampler2D u_sampler;
in vec2 v_texCoord;
out vec4 v_FragColor;

void main()
{
    v_FragColor = vec4(texture(u_sampler, v_texCoord).rgb, 1.0);
}
