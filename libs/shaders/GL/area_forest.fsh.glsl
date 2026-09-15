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

// Analytic forest: sparse tree crowns seen from above, drawn as the scalloped outline of three overlapping lobes of
// random sizes. v_maskTexCoords is the world-anchored lattice (1.0 == one 16px tile), with at most one crown per tile.
// A crown outline with 1px of anti-aliasing stays inside its tile, so tile and feature borders, where the tile hashes
// change, don't cut it. Outlines shade the fill like the stipple dots, with fwidth() anti-aliasing. Crowns of a few
// pixels would read as noise, so they fade into their average coverage, an even tint.

const float kEmptyTiles = 0.45;   // share of tiles without a crown
const float kLobeOffsetPx = 1.5;  // distance of the lobe centers from the crown center
const float kMinLobeRadiusPx = 1.2;
const float kMaxLobeRadiusPx = 2.8;
const float kHalfWidthPx = 0.5;   // outline half width
const float kMaxCrownRadiusPx = kLobeOffsetPx + kMaxLobeRadiusPx + kHalfWidthPx;
const float kMaxShiftPx = 7.0 - kMaxCrownRadiusPx;  // half a tile minus 1px of anti-aliasing
const float kDarken = 0.88;       // fill multiplier under an outline on a light fill
const float kMeanCoverage = 0.05; // average outline coverage at the base scale
const float kFadeStartPx = 2.5;   // lattice px per screen px
const float kFadeEndPx = 4.0;

// Per-tile random values in [0, 1). Unlike the stipple hash, it has no short period over integer cells.
vec3 Hash3(vec2 p)
{
  vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
  p3 += dot(p3, p3.yxz + 33.33);
  return fract((p3.xxy + p3.yzz) * p3.zyx);
}

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif

  vec2 fw = fwidth(v_maskTexCoords) * 16.0;
  float aa = max(fw.x, fw.y);

  // Fully faded fragments need no crown, and most others lie in an empty tile or away from its crown.
  float outline = 0.0;
  if (aa < kFadeEndPx)
  {
    vec3 h = Hash3(floor(v_maskTexCoords));  // xy: crown position; z: presence and rotation; all: lobe radii
    vec2 p = (fract(v_maskTexCoords) - 0.5) * 16.0 - (h.xy * 2.0 - 1.0) * kMaxShiftPx;  // px from the crown center
    float reach = kMaxCrownRadiusPx + aa;
    if (h.z >= kEmptyTiles && dot(p, p) < reach * reach)
    {
      vec3 radii = mix(vec3(kMinLobeRadiusPx), vec3(kMaxLobeRadiusPx), fract(h.yxz * vec3(7.0, 11.0, 17.0)));
      float angle = 6.2831853 * fract(h.z * 5.0);
      vec2 u = kLobeOffsetPx * vec2(cos(angle), sin(angle));
      vec2 v = vec2(-0.5 * u.x - 0.8660254 * u.y, 0.8660254 * u.x - 0.5 * u.y);  // u rotated by 120 degrees
      float sdf = min(min(length(p - u) - radii.x, length(p - v) - radii.y), length(p + u + v) - radii.z);
      outline = 1.0 - smoothstep(kHalfWidthPx - aa, kHalfWidthPx + aa, abs(sdf));
    }
  }
  float coverage = mix(outline, kMeanCoverage, smoothstep(kFadeStartPx, kFadeEndPx, aa));

  color.rgb = ModulateByPatternDots(color.rgb, kDarken, coverage);
  color.a *= u_opacity;
  v_FragColor = color;
}
