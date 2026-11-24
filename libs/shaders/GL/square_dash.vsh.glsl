layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_colorTexCoord;
layout (location = 3) in vec4 a_range;
layout (location = 4) in vec2 a_pattern;

layout (location = 0) out vec2 v_colorTexCoord;
layout (location = 1) out vec4 v_dash;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_isOutlinePass;
};

void main()
{
  float scale = length(vec4(kShapeCoordScalar, 0, 0, 0) * u_modelView);
  float pixelToPeriod = 1.0 / (scale * a_pattern.y);
  vec2 offset = a_normal.xy;
  vec2 axis = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  vec2 transformedAxisPos = calcLineTransformedAxisPos(axis, a_position.xy + offset, u_modelView, length(offset));
  float h = a_range.w * pixelToPeriod;
  v_dash = vec4(a_range.x + a_normal.z * pixelToPeriod + h, a_pattern.x + 2.0 * h, a_range.yz);
  v_colorTexCoord = a_colorTexCoord;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
