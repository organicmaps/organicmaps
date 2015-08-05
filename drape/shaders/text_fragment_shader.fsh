varying vec2 v_colorTexCoord;
varying vec2 v_outlineColorTexCoord;
varying vec2 v_maskTexCoord;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;

const lowp float OUTLINE_MIN_VALUE0 = 0.41;
const lowp float OUTLINE_MIN_VALUE1 = 0.565;
const lowp float OUTLINE_MAX_VALUE0 = 0.57;
const lowp float OUTLINE_MAX_VALUE1 = 0.95;
const lowp float GLYPH_MIN_VALUE = 0.45;
const lowp float GLYPH_MAX_VALUE = 0.6;

vec4 colorize(vec4 base, vec4 outline, float alpha)
{
  if (alpha > OUTLINE_MAX_VALUE1)
    return vec4(outline.rgb, 0);
  if (alpha > OUTLINE_MAX_VALUE0)
  {
    float oFactor = smoothstep(OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, alpha);
    return mix(vec4(outline.rgb,0), outline, oFactor);
  }
  if (alpha > OUTLINE_MIN_VALUE1)
  {
    return outline;
  }
  if (alpha > OUTLINE_MIN_VALUE0)
  {
    float oFactor = smoothstep(OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, alpha);
    return mix(base, outline, oFactor);
  }
  return base;
}

vec4 without_outline(vec4 base, float alpha)
{
  if (alpha > GLYPH_MIN_VALUE)
  {
    float oFactor = smoothstep(GLYPH_MIN_VALUE, GLYPH_MAX_VALUE, alpha);
    return mix(base, vec4(0, 0, 0, 0), oFactor);
  }
  return base;
}

void main (void)
{
  vec4 base = texture2D(u_colorTex, v_colorTexCoord);
  vec4 outline = texture2D(u_colorTex, v_outlineColorTexCoord);
  float alpha = texture2D(u_maskTex, v_maskTexCoord).a;

  vec4 finalColor;
  if (outline.a > 0.1)
    finalColor = colorize(base, outline, 1.0 - alpha);
  else
    finalColor = without_outline(base, 1.0 - alpha);

  finalColor.a *= u_opacity;
  gl_FragColor = finalColor;
}
