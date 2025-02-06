#ifdef ENABLE_VTF
layout (location = 0) in LOW_P vec4 v_color;
#else
layout (location = 1) in vec2 v_colorTexCoord;
layout (binding = 1) uniform sampler2D u_colorTex;
#endif
layout (location = 2) in vec2 v_maskTexCoord;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  vec2 u_contrastGamma;
  vec2 u_position;
  float u_isOutlinePass;
  float u_opacity;
  float u_length;
};

layout (binding = 2) uniform sampler2D u_maskTex;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 glyphColor = v_color;
#else
  LOW_P vec4 glyphColor = texture(u_colorTex, v_colorTexCoord);
#endif
  float dist = texture(u_maskTex, v_maskTexCoord).r;
  float alpha = smoothstep(u_contrastGamma.x - u_contrastGamma.y, u_contrastGamma.x + u_contrastGamma.y, dist) * u_opacity;
  glyphColor.a *= alpha;
  v_FragColor = glyphColor;
}
