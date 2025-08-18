layout (location = 0) in vec2 v_colorTexCoords;
layout (location = 1) in float v_intensity;

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
  vec4 finalColor = vec4(texture(u_colorTex, v_colorTexCoords).rgb, u_opacity);
  v_FragColor = vec4((v_intensity * 0.2 + 0.8) * finalColor.rgb, finalColor.a);
}
