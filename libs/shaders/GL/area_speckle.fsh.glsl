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

// Analytic speckle: a denser, finer, size-varied dot field for rocky surfaces (scree/bare_rock). Same
// single-pass solid-fill + darken idea as the stipple, but smaller cells and per-cell radius variation
// read as a coarse, irregular rock texture rather than even sand grains. fwidth() AA, no texture/mip.

const float kCellPx = 4.0;        // denser than stipple (divides the 16px tile, so cells tile seamlessly)
const float kBaseRadiusPx = 0.7;
const float kRadiusVar = 0.6;     // per-cell radius variation -> irregular sizes
const float kJitter = 0.5;        // keep kJitter*0.5*kCellPx + maxRadius < kCellPx*0.5
const float kDarken = 0.78;

// Integer hash (see area_forest.fsh.glsl) keyed on the integer cell + a salt: exact at any index, so the
// coarse anchor (DrawPatternArea) keeps the cell seam-consistent across map tiles without banding.
float Hash(ivec2 c, uint salt)
{
  uint h = uint(c.x) * 0x9E3779B1u ^ uint(c.y) * 0x85EBCA77u ^ salt * 0xC2B2AE3Du;
  h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15; h *= 0x846CA68Bu; h ^= h >> 16;
  return float(h >> 8) * (1.0 / 16777216.0);
}

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif

  vec2 px = v_maskTexCoords * 16.0;
  ivec2 cell = ivec2(floor(px / kCellPx));
  vec2 toCenter =
      (fract(px / kCellPx) - 0.5) * kCellPx - (vec2(Hash(cell, 0u), Hash(cell, 20u)) - 0.5) * (kJitter * kCellPx);
  float radius = kBaseRadiusPx * (1.0 - kRadiusVar * 0.5 + kRadiusVar * Hash(cell, 4u));
  float d = length(toCenter);
  float aa = max(fwidth(px.x), fwidth(px.y));
  float coverage = 1.0 - smoothstep(radius - aa, radius + aa, d);

  color.rgb *= mix(1.0, kDarken, coverage);
  color.a *= u_opacity;
  v_FragColor = color;
}
