layout (location = 0) in vec4 a_position;
layout (location = 1) in vec2 a_normal;
layout (location = 2) in vec2 a_colorTexCoords;
layout (location = 3) in vec2 a_maskTexCoords;

layout (location = 0) out vec2 v_colorTexCoords;
layout (location = 1) out vec2 v_maskTexCoords;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

void main()
{
  vec4 pivot = vec4(a_position.xyz, 1.0) * u_modelView;
  vec4 offset = vec4(a_normal, 0.0, 0.0) * u_projection;
  gl_Position = applyBillboardPivotTransform(pivot * u_projection, u_pivotTransform,
                                             a_position.w * u_zScale, offset.xy);
  v_colorTexCoords = a_colorTexCoords;
  v_maskTexCoords = a_maskTexCoords;
}
