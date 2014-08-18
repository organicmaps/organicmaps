attribute highp vec4 a_position;
attribute lowp vec4 a_texcoord;
attribute lowp vec4 a_color;
attribute lowp vec4 a_outline_color;

uniform highp mat4 modelView;
uniform highp mat4 projection;

varying lowp vec3 v_texcoord;
varying lowp vec4 v_color;
varying lowp vec4 v_outline_color;

void main()
{
  gl_Position = (vec4(a_position.zw, 0, 0) + (vec4(a_position.xy, a_texcoord.w, 1) * modelView)) * projection;

  v_texcoord = a_texcoord.xyz;
  v_color = a_color;
  v_outline_color = a_outline_color;
}
