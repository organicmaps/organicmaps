uniform sampler2D font_texture;

varying vec2 v_texcoord;
varying vec4 v_color;

void main (void)
{
	vec4 color = texture2D(font_texture, v_texcoord);
	gl_FragColor = vec4(v_color.rgb, v_color.a*color.a);
}                    