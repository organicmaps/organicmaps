varying vec2 v_colorTexCoord;
varying vec2 v_outlineColorTexCoord;
varying vec2 v_maskTexCoord;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;
uniform float u_opacity;
uniform vec4 u_outlineGlyphParams;
uniform vec2 u_glyphParams;

vec4 colorize(vec4 base, vec4 outline, float alpha)
{
  if (alpha > u_outlineGlyphParams.w)
    return vec4(outline.rgb, 0);
  if (alpha > u_outlineGlyphParams.z)
  {
    float oFactor = smoothstep(u_outlineGlyphParams.w, u_outlineGlyphParams.z, alpha);
    return mix(vec4(outline.rgb,0), outline, oFactor);
  }
  if (alpha > u_outlineGlyphParams.y)
  {
    return outline;
  }
  if (alpha > u_outlineGlyphParams.x)
  {
    float oFactor = smoothstep(u_outlineGlyphParams.x, u_outlineGlyphParams.y, alpha);
    return mix(base, outline, oFactor);
  }
  return base;
}

vec4 without_outline(vec4 base, float alpha)
{
  if (alpha > u_glyphParams.x)
  {
    float oFactor = smoothstep(u_glyphParams.x, u_glyphParams.y, alpha);
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
