varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying float v_halfLength;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform float u_outline;

const float kAntialiasingThreshold = 0.92;

const float kOutlineThreshold1 = 0.8;
const float kOutlineThreshold2 = 0.5;

const vec3 kLightArrow = vec3(1.0, 1.0, 1.0);
const vec3 kDarkArrow = vec3(107.0 / 255.0, 81.0 / 255.0, 20.0 / 255.0);

void main(void)
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  float alphaCode = color.a;
  vec4 mask = texture2D(u_maskTex, v_maskTexCoord);
  color.a = u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_halfLength)));
  color.rgb = mix(color.rgb, mask.rgb * mix(kLightArrow, kDarkArrow, step(alphaCode, 0.6)), mask.a);
  if (u_outline > 0.0)
  {
    color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), step(kOutlineThreshold1, abs(v_halfLength)));
    color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(v_halfLength)));
  }
  gl_FragColor = color;
}
