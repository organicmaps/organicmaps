attribute vec3 a_normal;
attribute vec3 a_position;
attribute vec4 a_color;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

varying vec3 v_radius;
varying vec4 v_color;

void main()
{
  vec3 radius = a_normal * a_position.z;
  vec4 pos = vec4(a_position.xy, 0, 1) * u_modelView;
  vec4 shiftedPos = vec4(radius.xy, 0, 0) + pos;
  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);

  v_radius = radius;
  v_color = a_color;
}
