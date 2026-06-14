#ifdef ENABLE_VTF
layout (location = 0) in LOW_P vec4 v_color;
#else
layout (location = 1) in vec2 v_colorTexCoords;
layout (binding = 1) uniform sampler2D u_colorTex;
#endif
layout (location = 2) in vec2 v_maskTexCoords;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

// Deciduous forest (landuse=forest|deciduous): the broadleaf glyph only. Same analytic scatter, seam
// handling and constants as the mixed forest - see area_forest.fsh.glsl for the full commentary.

const float kCellPx = 32.0;      // MUST match the forest tile size in AreaShape::DrawPatternArea
const float kJitter = 0.7;
const float kGlyphScale = 1.1;
const float kDensity = 0.32;
const vec3 kTint = vec3(0.88, 0.93, 0.85);

float Hash(ivec2 c, uint salt)
{
  uint h = uint(c.x) * 0x9E3779B1u ^ uint(c.y) * 0x85EBCA77u ^ salt * 0xC2B2AE3Du;
  h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15; h *= 0x846CA68Bu; h ^= h >> 16;
  return float(h >> 8) * (1.0 / 16777216.0);
}

float sdCircle(vec2 p, vec2 c, float r)
{
  return length(p - c) - r;
}

float sdBox(vec2 p, vec2 c, vec2 h)
{
  vec2 d = abs(p - c) - h;
  return length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0);
}

// Broadleaf: three overlapping canopy circles (narrow crown) + a trunk box. +y is up.
float broadleafSDF(vec2 q)
{
  float d = sdCircle(q, vec2(0.0, 0.15), 0.23);
  d = min(d, sdCircle(q, vec2(-0.13, -0.04), 0.21));
  d = min(d, sdCircle(q, vec2(0.13, -0.04), 0.21));
  d = min(d, sdBox(q, vec2(0.0, -0.34), vec2(0.04, 0.12)));
  return d;
}

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif

  vec2 px = v_maskTexCoords * kCellPx;
  ivec2 baseCell = ivec2(floor(v_maskTexCoords));
  float aaPx = max(fwidth(px.x), fwidth(px.y));

  float coverage = 0.0;
  for (int j = -1; j <= 1; ++j)
  {
    for (int i = -1; i <= 1; ++i)
    {
      ivec2 cell = baseCell + ivec2(i, j);
      if (Hash(cell, 23u) > kDensity)
        continue;
      float fp = kCellPx * kGlyphScale * (0.8 + 0.4 * Hash(cell, 29u));
      vec2 center = (vec2(cell) + 0.5 + (vec2(Hash(cell, 0u), Hash(cell, 41u)) - 0.5) * kJitter) * kCellPx;
      vec2 q = (px - center) / fp;
      if (abs(q.x) > 0.6 || abs(q.y) > 0.65)
        continue;
      float d = broadleafSDF(q);
      float aa = max(aaPx / fp, 0.003);
      coverage = max(coverage, 1.0 - smoothstep(-aa, aa, d));
    }
  }

  color.rgb = mix(color.rgb, color.rgb * kTint, coverage);
  color.a *= u_opacity;
  v_FragColor = color;
}
