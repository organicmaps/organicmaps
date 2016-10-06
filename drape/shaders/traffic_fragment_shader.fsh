varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying float v_halfLength;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

const float kAntialiasingThreshold = 0.92;

void main(void)
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  vec4 mask = texture2D(u_maskTex, v_maskTexCoord);
  color.a = color.a * u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_halfLength)));
  color.rgb = mix(color.rgb, mask.rgb, mask.a);

  gl_FragColor = color;
}
