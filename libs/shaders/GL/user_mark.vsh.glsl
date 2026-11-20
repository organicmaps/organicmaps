layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normalAndDefaultDepth;
layout (location = 2) in vec4 a_texCoords;
layout (location = 3) in vec4 a_color;

layout (location = 0) out vec4 v_texCoords;
layout (location = 1) out vec4 v_maskColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_isOutlinePass;
  float u_bookmarkScale;
};

void main()
{
  vec2 normal = u_bookmarkScale * a_normalAndDefaultDepth.xy;
  vec4 p = vec4(a_position, 1.0) * u_modelView;
  vec4 pos = vec4(normal, 0.0, 0.0) + p;
  vec4 projectedPivot = p * u_projection;
  gl_Position = applyPivotTransform(pos * u_projection, u_pivotTransform, 0.0);
  float newZ = projectedPivot.y / projectedPivot.w * 0.5 + 0.5;
  gl_Position.z = mix(gl_Position.z, newZ, a_normalAndDefaultDepth.z);
  v_texCoords = a_texCoords;
  v_maskColor = a_color;
}
