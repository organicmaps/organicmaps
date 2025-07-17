layout (location = 0) in vec4 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec2 a_colorTexCoord;

layout (location = 0) out vec2 v_colorTexCoord;
layout (location = 1) out vec3 v_radius;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_trafficParams;
  vec4 u_outlineColor;
  vec4 u_lightArrowColor;
  vec4 u_darkArrowColor;
  float u_outline;
  float u_opacity;
};

void main()
{
  vec2 normal = a_normal.xy;
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  int index = int(a_position.w);
  float leftSize = u_lightArrowColor[index];
  float rightSize = u_darkArrowColor[index];
  if (dot(normal, normal) != 0.0)
  {
    // offset by normal = rightVec * (rightSize - leftSize) / 2
    vec2 norm = normal * 0.5 * (rightSize - leftSize);
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
  }
  // radius = (leftSize + rightSize) / 2
  v_radius = vec3(a_normal.zw, 1.0) * 0.5 * (leftSize + rightSize);
  vec2 finalPos = transformedAxisPos + v_radius.xy;
  v_colorTexCoord = a_colorTexCoord;
  vec4 pos = vec4(finalPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
