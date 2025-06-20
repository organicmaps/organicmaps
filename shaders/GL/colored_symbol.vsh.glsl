layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec4 a_colorTexCoords;

layout (location = 0) out vec4 v_normal;
#ifdef ENABLE_VTF
layout (location = 1) out LOW_P vec4 v_color;
#else
layout (location = 2) out vec2 v_colorTexCoords;
#endif

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

#ifdef ENABLE_VTF
layout (binding = 1) uniform sampler2D u_colorTex;
#endif

void main()
{
  vec4 p = vec4(a_position, 1) * u_modelView;
  vec4 pos = vec4(a_normal.xy + a_colorTexCoords.zw, 0, 0) + p;
  gl_Position = applyPivotTransform(pos * u_projection, u_pivotTransform, 0.0);
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoords.xy);
#else
  v_colorTexCoords = a_colorTexCoords.xy;
#endif
  v_normal = a_normal;
}
