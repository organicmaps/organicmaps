varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying float v_halfLength;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform float u_outline;

uniform vec3 u_lightArrowColor;
uniform vec3 u_darkArrowColor;
uniform vec3 u_outlineColor;

const float kAntialiasingThreshold = 0.92;

const float kOutlineThreshold1 = 0.8;
const float kOutlineThreshold2 = 0.5;

const float kMaskOpacity = 0.7;

void main()
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  float alphaCode = color.a;
  vec4 mask = texture2D(u_maskTex, v_maskTexCoord);
  color.a = u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_halfLength)));
  color.rgb = mix(color.rgb, mask.rgb * mix(u_lightArrowColor, u_darkArrowColor, step(alphaCode, 0.6)), mask.a * kMaskOpacity);
  if (u_outline > 0.0)
  {
    color.rgb = mix(color.rgb, u_outlineColor, step(kOutlineThreshold1, abs(v_halfLength)));
    color.rgb = mix(color.rgb, u_outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_halfLength)));
  }
  gl_FragColor = color;
}
