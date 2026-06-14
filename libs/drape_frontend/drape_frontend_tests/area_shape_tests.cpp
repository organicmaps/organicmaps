#include "testing/testing.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include <cmath>
#include <string>

namespace
{
double Frac(double v)
{
  return v - std::floor(v);
}

// Reproduces the GPU mask UV that DrawHatchingArea bakes into the vertex buffer:
//   maxU * (worldCoord - anchor),  maxU = baseGtoPScale / maskSizePx.
double HatchUV(double worldCoord, double bboxMin, uint32_t maskSizePx, double baseGtoPScale)
{
  double const maxU = baseGtoPScale / maskSizePx;
  return maxU * (worldCoord - df::CalcHatchingPhaseAnchor(bboxMin, maskSizePx, baseGtoPScale));
}
}  // namespace

// The analytic hatch repeats with mod() over the lattice coordinate, so only its fractional part (the
// phase) is visible. The phase must be a function of the world coordinate alone — independent of how a
// feature happens to be clipped into per-tile bounding boxes — otherwise the pattern snaps/shifts and
// breaks at tile seams when geometry is re-tiled or the LOD changes (issue #12804).
UNIT_TEST(HatchingPhaseAnchor_StableAcrossTileClipping)
{
  uint32_t const kMaskPx = 16;                                      // analytic hatch tile is 16px (kHatchTilePx)
  double const kBaseGtoP = 4096.0;                                  // pixels per mercator unit at some zoom level
  double const kPeriod = static_cast<double>(kMaskPx) / kBaseGtoP;  // world units per mask repeat

  // A world vertex lying on the shared edge of two adjacent tiles.
  double const worldX = 123.4567;

  // The two tiles clip the same feature differently => arbitrary, non-period-aligned bbox mins.
  double const bboxMinA = 123.40;
  double const bboxMinB = 122.91;

  double const anchorA = df::CalcHatchingPhaseAnchor(bboxMinA, kMaskPx, kBaseGtoP);
  double const anchorB = df::CalcHatchingPhaseAnchor(bboxMinB, kMaskPx, kBaseGtoP);

  // Anchor sits on the global period grid, just below the bbox min (so UVs stay small for float precision).
  TEST(anchorA <= bboxMinA && bboxMinA - anchorA < kPeriod, (anchorA, bboxMinA, kPeriod));
  TEST(anchorB <= bboxMinB && bboxMinB - anchorB < kPeriod, (anchorB, bboxMinB, kPeriod));

  // The invariant: the sampled texel (UV mod 1) for the shared vertex matches across both tiles.
  double const uvA = HatchUV(worldX, bboxMinA, kMaskPx, kBaseGtoP);
  double const uvB = HatchUV(worldX, bboxMinB, kMaskPx, kBaseGtoP);
  TEST_ALMOST_EQUAL_ABS(Frac(uvA), Frac(uvB), 1e-5, (uvA, uvB));

  // Guard against regression: the previous bbox-anchored formula did NOT have this property.
  double const oldA = (kBaseGtoP / kMaskPx) * (worldX - bboxMinA);
  double const oldB = (kBaseGtoP / kMaskPx) * (worldX - bboxMinB);
  TEST_GREATER(std::abs(Frac(oldA) - Frac(oldB)), 1e-3, (oldA, oldB));
}

// Phase stability must also hold when the same feature is rendered at different zoom levels: at a fixed
// world coordinate the texel depends only on baseGtoPScale, not on the (clipped) bbox.
UNIT_TEST(HatchingPhaseAnchor_IndependentOfBBoxAtFixedScale)
{
  uint32_t const kMaskPx = 16;
  double const kBaseGtoP = 256.0;
  double const worldX = -57.318;  // negative coordinate => floor() must round toward -inf, not toward 0

  // Many distinct clip bboxes, all containing worldX, must yield the same phase.
  double const refUV = HatchUV(worldX, worldX - 0.001, kMaskPx, kBaseGtoP);
  for (double offset : {0.0123, 0.5, 1.7, 13.0, 100.25})
  {
    double const bboxMin = worldX - offset;
    double const uv = HatchUV(worldX, bboxMin, kMaskPx, kBaseGtoP);
    TEST_ALMOST_EQUAL_ABS(Frac(uv), Frac(refUV), 1e-4, (offset, uv, refUV));
  }
}

// Within a DepthLayer, render groups draw in dp::RenderState order (RenderGroupComparator falls back to
// RenderState::operator<, which for a shared layer and blending compares the gpu::Program value). The
// analytic solid-fill patterns (stipple/speckle/grid) are opaque fills, but a hatch overlay is transparent
// between its lines yet still writes depth across its whole quad (depth-write is bound to the depth test).
// So a fill whose group is drawn AFTER the hatch is depth-culled wherever they overlap -- e.g. a landcover
// fill under a protected-area hatch. The fills must therefore sort BEFORE the hatch programs; this guards
// the regression of declaring a solid-fill pattern program after HatchingArea in programs.hpp.
UNIT_TEST(AreaPatterns_SolidFillsDrawBeforeHatchOverlay)
{
  auto const state = [](gpu::Program p) { return df::CreateRenderState(p, df::DepthLayer::GeometryLayer); };

  // gpu::DebugPrint returns string_view, which base::Message can't concatenate; wrap names in std::string.
  for (auto fill : {gpu::Program::AreaStipple, gpu::Program::AreaSpeckle, gpu::Program::AreaGrid})
    for (auto hatch : {gpu::Program::HatchingArea, gpu::Program::HatchingAreaDash})
      TEST(state(fill) < state(hatch),
           (std::string{DebugPrint(fill)}, "must sort before", std::string{DebugPrint(hatch)}));
}
