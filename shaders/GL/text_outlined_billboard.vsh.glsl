layout (location = 0) in vec2 a_colorTexCoord;
layout (location = 1) in vec2 a_outlineColorTexCoord;
layout (location = 2) in vec2 a_maskTexCoord;
layout (location = 3) in vec4 a_position;
layout (location = 4) in vec2 a_normal;

#ifdef ENABLE_VTF
layout (location = 0) out LOW_P vec4 v_color;
#else
layout (location = 1) out vec2 v_colorTexCoord;
#endif
layout (location = 2) out vec2 v_maskTexCoord;

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

layout (binding = 1) uniform sampler2D u_colorTex;

const float kBaseDepthShift = -10.0;

void main()
{
  float isOutline = step(0.5, u_isOutlinePass);
  float depthShift = kBaseDepthShift * isOutline;
  vec4 pivot = (vec4(a_position.xyz, 1.0) + vec4(0.0, 0.0, depthShift, 0.0)) * u_modelView;
  vec4 offset = vec4(a_normal, 0.0, 0.0) * u_projection;
  gl_Position = applyBillboardPivotTransform(pivot * u_projection, u_pivotTransform,
                                             a_position.w * u_zScale, offset.xy);
  vec2 colorTexCoord = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline);
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, colorTexCoord);
#else
  v_colorTexCoord = colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
