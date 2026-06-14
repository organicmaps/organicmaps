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

// Analytic forest: scattered tree symbols, broadleaf and pine mixed per cell, each built from closed-form
// SDF primitives (circles + box for the deciduous crown, triangle tiers for the fir). The glyph is
// computed, not sampled from a fixed-resolution atlas, so it stays crisp at ANY zoom (no upsampling at
// z19-20) and the solid fill avoids the thin-outline contrast wobble. Trees sit on a 32px cell - twice
// the other patterns - for larger, well-spaced symbols (see kForestTilePx in AreaShape::DrawPatternArea).
// Seam handling: the per-cell scatter keys on the integer cell index floor(v_maskTexCoords). To keep that
// index identical on both sides of a map-tile seam (CalcHatchingPhaseAnchor only stabilizes the fractional
// phase, not the integer part), DrawPatternArea anchors the forest to a COARSE global grid (kPatternWrap
// tiles); the hash below is an integer finalizer so it stays well-distributed for the large indices.

const float kCellPx = 32.0;      // MUST match the forest tile size in AreaShape::DrawPatternArea
const float kJitter = 0.7;
const float kGlyphScale = 1.1;   // tree footprint vs cell
const float kDensity = 0.32;     // fraction of cells that carry a tree (low -> rare, spaced out)
const vec3 kTint = vec3(0.88, 0.93, 0.85);  // very gentle darken -> faint, low-contrast symbols

// Integer hash (Wellons' lowbias32 finalizer) keyed on the integer cell + a salt -> [0,1). Exact at any
// index, so unlike fract(cell * k) it does not band once the coarse anchor makes the index large.
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

float sdTriangle(vec2 p, vec2 p0, vec2 p1, vec2 p2)
{
  vec2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
  vec2 v0 = p - p0, v1 = p - p1, v2 = p - p2;
  vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / dot(e0, e0), 0.0, 1.0);
  vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / dot(e1, e1), 0.0, 1.0);
  vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / dot(e2, e2), 0.0, 1.0);
  float s = sign(e0.x * e2.y - e0.y * e2.x);
  vec2 d = min(min(vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
                   vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
               vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
  return -sqrt(d.x) * sign(d.y);
}

// Trees in a normalized, centred coordinate (+y is up / north). Broadleaf: three overlapping canopy
// circles (the bottom pair close together for a narrow crown) + a trunk box. Pine: two stacked triangle
// tiers + a trunk. Both fit inside |q| < 0.5 so the 3x3 scatter never clips them.
float broadleafSDF(vec2 q)
{
  float d = sdCircle(q, vec2(0.0, 0.15), 0.23);
  d = min(d, sdCircle(q, vec2(-0.13, -0.04), 0.21));
  d = min(d, sdCircle(q, vec2(0.13, -0.04), 0.21));
  d = min(d, sdBox(q, vec2(0.0, -0.34), vec2(0.04, 0.12)));
  return d;
}

float pineSDF(vec2 q)
{
  float d = sdTriangle(q, vec2(0.0, 0.42), vec2(-0.24, 0.04), vec2(0.24, 0.04));
  d = min(d, sdTriangle(q, vec2(0.0, 0.18), vec2(-0.30, -0.24), vec2(0.30, -0.24)));
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

  vec2 px = v_maskTexCoords * kCellPx;  // base pixels (the tile size cancels baseGtoPScale)
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
      float d = Hash(cell, 13u) < 0.5 ? broadleafSDF(q) : pineSDF(q);  // mix pine and broadleaf per cell
      float aa = max(aaPx / fp, 0.003);
      coverage = max(coverage, 1.0 - smoothstep(-aa, aa, d));
    }
  }

  color.rgb = mix(color.rgb, color.rgb * kTint, coverage);
  color.a *= u_opacity;
  v_FragColor = color;
}
