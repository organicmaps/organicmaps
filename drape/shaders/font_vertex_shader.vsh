attribute vec4 a_position;
attribute vec4 a_color;
attribute vec4 a_texcoord;

uniform highp mat4 modelView;
uniform highp mat4 projection;

varying vec2 v_texcoord;
varying vec4 v_color;

void main()
{
    float r = a_texcoord.w;
    vec2 dir = position.zw - position.xy;
    float len = length(dir);
    vec4 pos2 = vec4(position.xy, a_texcoord.z, 1) * modelView;
    vec4 direc = vec4(position.zw, a_texcoord.z, 1) * modelView;
    dir = direc.xy - pos2.xy;
    float l2 = length(dir);
    dir = normalize(dir);
    dir *= len * r;
    pos2 += vec4(dir, 0, 0);
    
    gl_Position = pos2 * projection;
    
    v_texcoord=a_texcoord;
	v_color=a_color;
}                           