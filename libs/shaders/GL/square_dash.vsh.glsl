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
  vec3 distances = a_range.xyz * scale;
  float dash = a_pattern.x, period = a_pattern.y;
  // Only dashes painted on this source segment can send a cap along its tangent.
  float first = floor(distances.y / period) * period;
  if (first + dash <= distances.y)
    first += period;
  float last = (ceil(distances.z / period) - 1.0) * period;
  float type = a_range.w;
  float extension = sign(type);
  // When a dash continues through a join, the body ends at the vertex.
  if ((type == -1.0 && first < distances.y) || (type == 1.0 && last + dash > distances.z))
    extension = 0.0;
  vec2 side = a_normal.xy;
  float h = length(side);
  vec2 offset = side + extension * a_normal.z * vec2(side.y, -side.x);
  vec2 axis = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  vec2 transformedAxisPos = calcLineTransformedAxisPos(axis, a_position.xy + offset, u_modelView, length(offset));
  v_dash = vec4(distances.x - first + (1.0 + extension) * h, period, dash + 2.0 * h, last - first);
  v_colorTexCoord = a_colorTexCoord;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
