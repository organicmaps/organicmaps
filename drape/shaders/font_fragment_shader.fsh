varying lowp vec3 v_texcoord;
varying lowp vec4 v_colors;
varying mediump float v_index;

~getTexel~

const lowp float OUTLINE_MIN_VALUE0 = 0.41;
const lowp float OUTLINE_MIN_VALUE1 = 0.565;
const lowp float OUTLINE_MAX_VALUE0 = 0.57;
const lowp float OUTLINE_MAX_VALUE1 = 0.95;
const lowp float GLYPH_MIN_VALUE = 0.45;
const lowp float GLYPH_MAX_VALUE = 0.6;

lowp vec4 colorize(lowp vec4 baseColor)
{
  int textureIndex = int(v_index);
  lowp vec4 outline = getTexel(textureIndex, v_colors.zw);
  lowp vec4 base = getTexel(textureIndex, v_colors.xy);
  lowp float alpha = 1.0 - baseColor.a;

  if (alpha > OUTLINE_MAX_VALUE1)
    return vec4(outline.rgb, 0);
  if (alpha > OUTLINE_MAX_VALUE0)
  {
    lowp float oFactor = smoothstep(OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, alpha );
    return mix(vec4(outline.rgb,0), outline, oFactor);
  }
  if (alpha > OUTLINE_MIN_VALUE1)
  {
    return outline;
  }
  if (alpha > OUTLINE_MIN_VALUE0)
  {
    lowp float oFactor = smoothstep(OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, alpha );
    return mix(base, outline, oFactor);
  }
  return base;
}

lowp vec4 without_outline(lowp vec4 baseColor)
{
  int textureIndex = int(v_index);
  lowp vec4 base = getTexel(textureIndex, v_colors.xy);
  lowp float alpha = 1.0 - baseColor.a;

  if (alpha > GLYPH_MIN_VALUE)
  {
    lowp float oFactor = smoothstep(GLYPH_MIN_VALUE, GLYPH_MAX_VALUE, alpha );
    return mix(base, vec4(1, 1, 1, 0), oFactor);
  }
  return base;
}

void main (void)
{
  int textureIndex = int(v_texcoord.z / 2.0);
  int textureIndex2 = int(v_index);
  lowp vec4 base = getTexel(textureIndex2, v_colors.xy);
  mediump float alpha = getTexel(textureIndex, v_texcoord.xy).a;

  lowp float needOutline = (fract(v_texcoord.z / 2.0)) * 2.0;
  if (needOutline > 0.5)
    gl_FragColor = colorize(vec4(base.rgb, base.a*alpha));
  else
    gl_FragColor = without_outline(vec4(base.rgb, base.a*alpha));
}                    
