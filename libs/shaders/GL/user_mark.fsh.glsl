// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented without discarding fragments from depth buffer.

layout (location = 0) in vec4 v_texCoords;
layout (location = 1) in vec4 v_maskColor;

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

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  vec4 color = texture(u_colorTex, v_texCoords.xy);
  vec4 bgColor = texture(u_colorTex, v_texCoords.zw) * vec4(v_maskColor.xyz, 1.0);
  vec4 finalColor = mix(color, mix(bgColor, color, color.a), bgColor.a);
  finalColor.a = clamp(color.a + bgColor.a, 0.0, 1.0) * u_opacity * v_maskColor.w;
  if (finalColor.a < 0.01)
    discard;
  v_FragColor = finalColor;
}
