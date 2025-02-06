layout (location = 0) in vec4 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec4 v_radius;
layout (location = 1) out vec4 v_color;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_routeParams;
  vec4 u_color;
  vec4 u_maskColor;
  vec4 u_outlineColor;
  vec4 u_fakeColor;
  vec4 u_fakeOutlineColor;
  vec2 u_fakeBorders;
  vec2 u_pattern;
  vec2 u_angleCosSin;
  float u_arrowHalfWidth;
  float u_opacity;
};

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
