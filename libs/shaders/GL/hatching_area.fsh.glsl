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

// Analytic 45-degree hatch (protected areas, national parks, etc.). v_maskTexCoords is a world-anchored
// lattice coordinate where 1.0 == one 16px tile (baked in AreaShape::DrawHatchingArea, continuous across
// tile seams). The diagonal lines are computed procedurally with fwidth() anti-aliasing, so they stay
// crisp at every fractional zoom and fade gracefully under minification - there is no mask texture to
// mip or alias (issue #12804). Replaces the sampled area-hatching.png: 1px lines along x+y, every 8px.

const float kPeriodPx = 8.0;     // distance between diagonal lines, in 'base' pixels
const float kHalfWidthPx = 0.7;  // half line width along the x+y axis (~1px perpendicular)

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoords);
#endif

  vec2 px = v_maskTexCoords * 16.0;
  float diag = px.x + px.y;
  float m = mod(diag, kPeriodPx);
  float dist = min(m, kPeriodPx - m);               // distance to the nearest line center
  // fwidth is |dFdx| + |dFdy|, and diag sums both axes, so aa is ~2 base px here - twice the canonical
  // one-pixel filter. The line is drawn as a soft ~3px band peaking at ~0.75 coverage, which keeps the
  // hatch a light tint (issue #4992). Halving aa makes it crisper and more prominent.
  float aa = fwidth(diag);
  float coverage = 1.0 - smoothstep(kHalfWidthPx - aa, kHalfWidthPx + aa, dist);

  // coverage is the line's alpha, rgb stays the surface colour: blending is straight
  // (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), so scaling rgb would apply coverage a second time.
  color.a *= coverage * u_opacity;
  v_FragColor = color;
}
