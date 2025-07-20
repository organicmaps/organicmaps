#ifdef ENABLE_VTF
layout (location = 0) in LOW_P vec4 v_color;
#else
layout (location = 1) in vec2 v_colorTexCoords;
layout (binding = 1) uniform sampler2D u_colorTex;
#endif

layout (location = 0) out vec4 v_FragColor;

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
#ifdef ENABLE_VTF
  LOW_P vec4 finalColor = v_color;
#else
  LOW_P vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
#endif
  finalColor.a *= u_opacity;
  v_FragColor = finalColor;
}
