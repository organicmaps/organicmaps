attribute vec3 a_normal;
attribute vec3 a_position;
attribute vec4 a_color;

uniform mat4 modelView;
uniform mat4 projection;

varying vec3 v_radius;
varying vec4 v_color;

void main(void)
{
  vec3 radius = a_normal * a_position.z;
  // Here we intentionally decrease precision of 'pos' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pos = vec4(a_position.xy, 0, 1) * modelView;
  highp vec4 shiftedPos = vec4(radius.xy, 0, 0) + pos;
  gl_Position = shiftedPos * projection;
  v_radius = radius;
  v_color = a_color;
}
