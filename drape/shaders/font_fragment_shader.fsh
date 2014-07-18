
uniform sampler2D u_textures[8];

varying vec3 v_texcoord;
varying vec4 v_color;
varying vec4 v_outline_color;

const int Index0  = 0;
const int Index1  = 1;
const int Index2  = 2;
const int Index3  = 3;
const int Index4  = 4;
const int Index5  = 5;
const int Index6  = 6;
const int Index7  = 7;

const float OUTLINE_MIN_VALUE0 = 0.52;
const float OUTLINE_MIN_VALUE1 = 0.58;
const float OUTLINE_MAX_VALUE0 = 0.64;
const float OUTLINE_MAX_VALUE1 = 0.68;

vec4 colorize(vec4 baseColor)
{
  vec4 outline = v_outline_color;
  float alpha = 1.0 - baseColor.a;

  if (alpha > OUTLINE_MAX_VALUE1)
    return vec4(outline.rgb,0);
  if (alpha > OUTLINE_MAX_VALUE0)
  {
    float oFactor=smoothstep(OUTLINE_MAX_VALUE1, OUTLINE_MAX_VALUE0, alpha );
    return mix(vec4(outline.rgb,0), outline, oFactor);
  }
  if (alpha > OUTLINE_MIN_VALUE1)
  {
    return outline;
  }
  if (alpha > OUTLINE_MIN_VALUE0)
  {
    float oFactor=smoothstep(OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, alpha );
    return mix(v_color, outline, oFactor);
  }
  return v_color;
}

vec4 without_outline(vec4 baseColor)
{
  vec4 outline = v_outline_color;
  float alpha = 1.0 - baseColor.a;

  if (alpha > OUTLINE_MIN_VALUE0)
  {
    float oFactor=smoothstep(OUTLINE_MIN_VALUE0, OUTLINE_MIN_VALUE1, alpha );
    return mix(v_color, vec4(1,1,1,0), oFactor);
  }
  return v_color;
}

void main (void)
{
  float alpha;
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

  float needOutline = (v_texcoord.z / 2.0 - floor(v_texcoord.z / 2.0)) * 2.0;
  if (needOutline > 0.5)
    gl_FragColor = colorize(vec4(v_color.rgb, v_color.a*alpha));
  else
    gl_FragColor = without_outline(vec4(v_color.rgb, v_color.a*alpha));
}                    
