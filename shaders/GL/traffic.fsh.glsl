layout (location = 0) in vec2 v_colorTexCoord;
layout (location = 1) in vec2 v_maskTexCoord;
layout (location = 2) in float v_halfLength;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_trafficParams;
  vec4 u_outlineColor;
  vec4 u_lightArrowColor;
  vec4 u_darkArrowColor;
  float u_outline;
  float u_opacity;
};

layout (binding = 1) uniform sampler2D u_colorTex;
layout (binding = 2) uniform sampler2D u_maskTex;

const float kAntialiasingThreshold = 0.92;
const float kOutlineThreshold1 = 0.8;
const float kOutlineThreshold2 = 0.5;
const float kMaskOpacity = 0.7;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  float alphaCode = color.a;
  vec4 mask = texture(u_maskTex, v_maskTexCoord);
  color.a = u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_halfLength)));
  color.rgb = mix(color.rgb, mask.rgb * mix(u_lightArrowColor.rgb, u_darkArrowColor.rgb, step(alphaCode, 0.6)), mask.a * kMaskOpacity);
  if (u_outline > 0.0)
  {
    color.rgb = mix(color.rgb, u_outlineColor.rgb, step(kOutlineThreshold1, abs(v_halfLength)));
    color.rgb = mix(color.rgb, u_outlineColor.rgb, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_halfLength)));
  }
  v_FragColor = color;
}
