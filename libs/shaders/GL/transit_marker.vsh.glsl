layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec4 v_offsets;
layout (location = 1) out vec4 v_color;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_params;
  float u_lineHalfWidth;
  float u_maxRadius;
};

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
