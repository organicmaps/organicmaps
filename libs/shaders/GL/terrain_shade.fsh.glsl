layout (location = 0) in float v_intensity;

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
  // The interpolated intensity is relative to the flat ground: negative - the slope faces
  // away from the light (black shadow), positive - towards it (white highlight). The max
  // alphas match the legacy per-triangle palette (96 and 40 of 255).
  const float kShadowMaxAlpha = 0.376;
  const float kHighlightMaxAlpha = 0.157;
  float shadow = clamp(-v_intensity, 0.0, 1.0);
  float highlight = clamp(v_intensity, 0.0, 1.0);
  LOW_P vec4 color = vec4(vec3(step(0.0, v_intensity)), shadow * kShadowMaxAlpha + highlight * kHighlightMaxAlpha);
  color.a *= u_opacity;
  v_FragColor = color;
}
