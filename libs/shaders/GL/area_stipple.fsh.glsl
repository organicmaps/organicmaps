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

// Analytic stipple: a solid surface fill (sand/beach/desert) speckled with subtly darker jittered dots.
// v_maskTexCoords is the world-anchored lattice (1.0 == one 16px tile, continuous across tile seams). The
// dots use fwidth() anti-aliasing, so they stay crisp at any zoom and fade to an even tint under
// minification - no texture, no mip, no aliasing (issue #12804). Unlike the hatches this is a single-pass
// fill that modulates the surface colour rather than a white-on-transparent mask.

const float kCellPx = 8.0;     // dot lattice cell (divides the 16px tile, so cells tile seamlessly)
const float kRadiusPx = 1.2;   // dot radius
const float kJitter = 0.55;    // keep kJitter*0.5*kCellPx + kRadiusPx < kCellPx*0.5 (dots stay in cell)
const float kDarken = 0.80;    // surface multiplier under a dot (gentle - texture should whisper)

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
  float d = length(toCenter);
  float aa = max(fwidth(px.x), fwidth(px.y));  // continuous coord: no fract-seam derivative spike
  float coverage = 1.0 - smoothstep(kRadiusPx - aa, kRadiusPx + aa, d);

  color.rgb *= mix(1.0, kDarken, coverage);
  color.a *= u_opacity;
  v_FragColor = color;
}
