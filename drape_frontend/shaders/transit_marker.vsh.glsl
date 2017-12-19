attribute vec3 a_position;
attribute vec4 a_normal;
attribute vec4 a_color;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

uniform float u_lineHalfWidth;
uniform vec2 u_angleCosSin;

varying vec4 v_offsets;
varying vec4 v_color;

void main()
{
  vec4 pos = vec4(a_position.xy, 0, 1) * modelView;
  vec2 normal = vec2(a_normal.x * u_angleCosSin.x - a_normal.y * u_angleCosSin.y,
                     a_normal.x * u_angleCosSin.y + a_normal.y * u_angleCosSin.x);
  vec2 shiftedPos = normal * u_lineHalfWidth + pos.xy;
  pos = vec4(shiftedPos, a_position.z, 1.0) * projection;
  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
  vec2 offsets = abs(a_normal.zw);
  v_offsets = vec4(a_normal.zw, offsets - 1.0);
  v_color = a_color;
}
