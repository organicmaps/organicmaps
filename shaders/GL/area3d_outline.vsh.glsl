layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_colorTexCoords;

#ifdef ENABLE_VTF
layout (location = 0) out LOW_P vec4 v_color;
#else
layout (location = 1) out vec2 v_colorTexCoords;
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
  vec4 pos = vec4(a_position, 1.0) * u_modelView;
  pos.xyw = (pos * u_projection).xyw;
  pos.z = a_position.z * u_zScale;
  gl_Position = u_pivotTransform * pos;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
}
