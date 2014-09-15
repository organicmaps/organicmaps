attribute vec2 a_position;
attribute vec4 a_texcoord;
attribute vec4 a_color;
attribute float a_index;

uniform highp mat4 modelView;
uniform highp mat4 projection;

varying vec3 v_texcoord;
varying lowp vec4 v_colors;
varying mediump float v_index;

void main()
{
  gl_Position = vec4(a_position, a_texcoord.w, 1) * modelView * projection;
    
  v_texcoord = a_texcoord.xyz;
  v_colors = a_color;
  v_index = a_index;
}
