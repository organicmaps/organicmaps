#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#endif

uniform sampler2D u_sampler;
varying vec2 v_texCoord;

void main()
{
    gl_FragColor = vec4(texture2D(u_sampler, v_texCoord).rgb, 1.0);
}
