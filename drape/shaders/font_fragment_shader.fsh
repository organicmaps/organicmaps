uniform sampler2D u_textures[8];

varying lowp vec3 v_texcoord;
varying lowp vec4 v_color;
varying lowp vec4 v_outline_color;

const int Index0  = 0;
const int Index1  = 1;
const int Index2  = 2;
const int Index3  = 3;
const int Index4  = 4;
const int Index5  = 5;
const int Index6  = 6;
const int Index7  = 7;

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
  lowp float alpha;
  int textureIndex = int(v_texcoord.z / 2.0);
  if (textureIndex == Index0)
    alpha = texture2D(u_textures[Index0], v_texcoord.xy).a;
  else if (textureIndex == Index1)
    alpha = texture2D(u_textures[Index1], v_texcoord.xy).a;
  else if (textureIndex == Index2)
    alpha = texture2D(u_textures[Index2], v_texcoord.xy).a;
  else if (textureIndex == Index3)
    alpha = texture2D(u_textures[Index3], v_texcoord.xy).a;
  else if (textureIndex == Index4)
    alpha = texture2D(u_textures[Index4], v_texcoord.xy).a;
  else if (textureIndex == Index5)
    alpha = texture2D(u_textures[Index5], v_texcoord.xy).a;
  else if (textureIndex == Index6)
    alpha = texture2D(u_textures[Index6], v_texcoord.xy).a;
  else if (textureIndex == Index7)
    alpha = texture2D(u_textures[Index7], v_texcoord.xy).a;

  lowp float needOutline = (fract(v_texcoord.z / 2.0)) * 2.0;
  if (needOutline > 0.5)
    gl_FragColor = colorize(vec4(v_color.rgb, v_color.a*alpha));
  else
    gl_FragColor = without_outline(vec4(v_color.rgb, v_color.a*alpha));
}                    
