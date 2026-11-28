layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_colorTexCoord;

#ifdef ENABLE_VTF
layout (location = 0) out LOW_P vec4 v_color;
#else
layout (location = 1) out vec2 v_colorTexCoords;
#endif
layout (location = 2) out vec2 v_maskTexCoords;

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

#ifdef ENABLE_VTF
layout (binding = 1) uniform sampler2D u_colorTex;
#endif

void main()
{
  // Expand the stroke in map coordinates so its world-anchored grain also covers caps and joins.
  float pixelsPerUnit = length((vec4(1, 0, 0, 0) * u_modelView).xy);
  vec2 point = a_position.xy + a_normal.xy / pixelsPerUnit;
  vec2 screen = (vec4(point, 0, 1) * u_modelView).xy;
  vec4 pos = vec4(screen, a_position.z, 1) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoord);
#else
  v_colorTexCoords = a_colorTexCoord;
#endif
  v_maskTexCoords = point * a_normal.z;
}
