in vec4 a_position;
in vec3 a_normal;
in vec4 a_color;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform vec2 u_angleCosSin;
uniform vec4 u_routeParams;

out vec4 v_radius;
out vec4 v_color;

void main()
{
  float r = u_routeParams.x * a_normal.z;
  vec2 normal = vec2(a_normal.x * u_angleCosSin.x - a_normal.y * u_angleCosSin.y,
                     a_normal.x * u_angleCosSin.y + a_normal.y * u_angleCosSin.x);
  vec4 radius = vec4(normal.xy * r, r, a_position.w);
  vec4 pos = vec4(a_position.xy, 0, 1) * u_modelView;
  vec2 shiftedPos = radius.xy + pos.xy;
  pos = vec4(shiftedPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  v_radius = radius;
  v_color = a_color;
}
