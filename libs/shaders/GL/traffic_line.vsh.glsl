layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_colorTexCoord;

layout (location = 0) out vec2 v_colorTexCoord;

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
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  v_colorTexCoord = a_colorTexCoord;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
