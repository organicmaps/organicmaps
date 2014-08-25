varying lowp vec3 v_texcoord;
varying lowp vec4 v_color;
varying lowp vec4 v_outline_color;

~getTexel~

const lowp float OUTLINE_MIN_VALUE0 = 0.41;
const lowp float OUTLINE_MIN_VALUE1 = 0.565;
const lowp float OUTLINE_MAX_VALUE0 = 0.57;
const lowp float OUTLINE_MAX_VALUE1 = 0.95;
const lowp float GLYPH_MIN_VALUE = 0.45;
const lowp float GLYPH_MAX_VALUE = 0.6;

lowp vec4 colorize(lowp vec4 baseColor)
{
  lowp vec4 outline = v_outline_color;
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
    return mix(v_color, outline, oFactor);
  }
  return v_color;
}

lowp vec4 without_outline(lowp vec4 baseColor)
{
  lowp vec4 outline = v_outline_color;
  lowp float alpha = 1.0 - baseColor.a;

  if (alpha > GLYPH_MIN_VALUE)
  {
    lowp float oFactor = smoothstep(GLYPH_MIN_VALUE, GLYPH_MAX_VALUE, alpha );
    return mix(v_color, vec4(1, 1, 1, 0), oFactor);
  }
  return v_color;
}

void main (void)
{
  int textureIndex = int(v_texcoord.z / 2.0);
  mediump float alpha = getTexel(textureIndex, v_texcoord.xy).a;

  lowp float needOutline = (fract(v_texcoord.z / 2.0)) * 2.0;
  if (needOutline > 0.5)
    gl_FragColor = colorize(vec4(v_color.rgb, v_color.a*alpha));
  else
    gl_FragColor = without_outline(vec4(v_color.rgb, v_color.a*alpha));
}                    
