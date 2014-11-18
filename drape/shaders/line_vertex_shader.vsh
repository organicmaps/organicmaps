#ifdef GL_FRAGMENT_PRECISION_HIGH
  #define MAXPREC highp
#else
  #define MAXPREC mediump
#endif

precision MAXPREC float;

attribute vec4 a_position;
attribute vec4 a_deltas;
attribute vec4 a_width_type;
attribute vec4 a_centres;
attribute vec3 a_color;
attribute vec3 a_mask;

varying float v_dx;
varying vec4 v_radius;
varying vec2 v_type;

varying vec3 v_color;
varying vec3 v_mask;

uniform mat4 modelView;
uniform mat4 projection;

void main(void)
{
  float r = abs(a_width_type.x);
  vec2 dir = a_position.zw - a_position.xy;
  float len = length(dir);
  vec4 pos2 = vec4(a_position.xy, a_deltas.z, 1) * modelView;
  vec4 direc = vec4(a_position.zw, a_deltas.z, 1) * modelView;
  dir = direc.xy - pos2.xy;
  float l2 = length(dir);
  dir = normalize(dir);
  dir *= len * r;
  pos2 += vec4(dir, 0, 0);

  gl_Position = pos2 * projection;

  v_dx = (a_deltas.y + a_deltas.x * r / l2 * len);
  v_radius.x = a_width_type.x;
  v_radius.y = r;
  v_radius.w = a_width_type.w;
  vec2 centr1 = (vec4(a_centres.xy, 0, 1) * modelView).xy;
  vec2 centr2 = (vec4(a_centres.zw, 0, 1) * modelView).xy;
  float len2 = length(centr1 - centr2);
  v_radius.z = len2;
  v_type = a_width_type.yz;

  v_color = a_color;
  v_mask = a_mask;
}
