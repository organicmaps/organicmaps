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

// Analytic grid: a regular dot lattice for planted landuse (orchard/vineyard) - trees/vines in rows. No
// jitter (the regularity is the signal), one dot per 16px tile. Same single-pass solid-fill + darken as
// the stipple, with fwidth() AA - crisp at any zoom, fades to an even tint under minification.

const float kCellPx = 16.0;    // one dot per tile -> sparse, regular rows
const float kRadiusPx = 1.6;
const float kDarken = 0.82;    // subtle

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif

  vec2 px = v_maskTexCoords * 16.0;
  vec2 toCenter = (fract(px / kCellPx) - 0.5) * kCellPx;  // regular lattice, no jitter
  float d = length(toCenter);
  float aa = max(fwidth(px.x), fwidth(px.y));
  float coverage = 1.0 - smoothstep(kRadiusPx - aa, kRadiusPx + aa, d);

  color.rgb *= mix(1.0, kDarken, coverage);
  color.a *= u_opacity;
  v_FragColor = color;
}
