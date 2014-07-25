attribute vec2 a_position;
attribute vec4 a_texcoord;
attribute vec4 a_color;
attribute vec4 a_outline_color;

uniform highp mat4 modelView;
uniform highp mat4 projection;

varying vec3 v_texcoord;
varying vec4 v_color;
varying vec4 v_outline_color;

void main()
{
  gl_Position = vec4(a_position, a_texcoord.w, 1) * modelView * projection;
    
  v_texcoord = a_texcoord.xyz;
  v_color = a_color;
  v_outline_color = a_outline_color;
}
