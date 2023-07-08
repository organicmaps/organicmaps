attribute vec3 a_position;
attribute vec4 a_normal;
attribute vec4 a_color;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform vec3 u_params;

varying vec4 v_offsets;
varying vec4 v_color;

void main()
{
  vec4 pos = vec4(a_position.xy, 0, 1) * u_modelView;
  vec2 normal = vec2(a_normal.x * u_params.x - a_normal.y * u_params.y,
                     a_normal.x * u_params.y + a_normal.y * u_params.x);
  vec2 shiftedPos = normal * u_params.z + pos.xy;
  pos = vec4(shiftedPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  vec2 offsets = abs(a_normal.zw);
  v_offsets = vec4(a_normal.zw, offsets - 1.0);
  v_color = a_color;
}
