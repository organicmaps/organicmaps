varying vec3 v_texcoord;
varying vec4 v_colors;
varying float v_index;

~getTexel~

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
    float oFactor = smoothstep(OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, alpha );
    return mix(vec4(outline.rgb,0), outline, oFactor);
  }
  if (alpha > OUTLINE_MIN_VALUE1)
  {
    return outline;
  }
  if (alpha > OUTLINE_MIN_VALUE0)
  {
    float oFactor = smoothstep(OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, alpha );
    return mix(base, outline, oFactor);
  }
  return base;
}

vec4 without_outline(vec4 base, float alpha)
{
  if (alpha > GLYPH_MIN_VALUE)
  {
    float oFactor = smoothstep(GLYPH_MIN_VALUE, GLYPH_MAX_VALUE, alpha );
    return mix(base, vec4(1, 1, 1, 0), oFactor);
  }
  return base;
}

void main (void)
{
  int shapeIndex = int(v_texcoord.z / 2.0);
  int colorIndex = int(v_index);
  vec4 base = getTexel(colorIndex, v_colors.xy);;
  vec4 outline = getTexel(colorIndex, v_colors.zw);
  float alpha = getTexel(shapeIndex, v_texcoord.xy).a;

  float needOutline = (fract(v_texcoord.z / 2.0)) * 2.0;
  vec4 finalColor;
  if (needOutline > 0.5)
    finalColor = colorize(base, outline, 1.0 - base.a*alpha);
  else
    finalColor = without_outline(base, 1.0 - base.a*alpha);

  gl_FragColor = finalColor;
}
