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

// Analytic dashed hatch (wetland). v_maskTexCoords is a world-anchored lattice coordinate where 1.0 ==
// one 16px tile (continuous across tile seams). Horizontal 8px dashes, 1px thick, on rows 8px apart,
// brick-staggered by half a period - matching the legacy dash-hatching.png but computed procedurally
// with fwidth() anti-aliasing, so it never shimmers on zoom-pan (issue #12804) and needs no mask texture.

const float kRowPeriodPx = 8.0;    // vertical distance between dash rows
const float kRowCenterPx = 3.5;    // first row center (matches the legacy tile)
const float kHalfThickPx = 0.5;    // half dash thickness -> ~1px
const float kDashPeriodPx = 16.0;  // dash + gap length
const float kDashHalfPx = 4.0;     // half dash length -> 8px dash

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif

  vec2 px = v_maskTexCoords * 16.0;

  // Horizontal rows.
  float ym = mod(px.y - kRowCenterPx, kRowPeriodPx);
  float yDist = min(ym, kRowPeriodPx - ym);
  float aaY = fwidth(px.y);
  float onRow = 1.0 - smoothstep(kHalfThickPx - aaY, kHalfThickPx + aaY, yDist);

  // Dashes along each row, staggered by row parity.
  float rowIdx = floor((px.y - kRowCenterPx) / kRowPeriodPx + 0.5);
  float xPhase = px.x + mod(rowIdx, 2.0) * (kDashPeriodPx * 0.5);
  float xDist = abs(mod(xPhase, kDashPeriodPx) - kDashHalfPx);
  float aaX = fwidth(px.x);
  float onDash = 1.0 - smoothstep(kDashHalfPx - aaX, kDashHalfPx + aaX, xDist);

  color *= onRow * onDash;                          // white-on-transparent mask, same as the legacy PNG
  color.a *= u_opacity;
  v_FragColor = color;
}
