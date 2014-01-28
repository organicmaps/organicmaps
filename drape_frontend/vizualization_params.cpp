#include "vizualization_params.hpp"

#include "../base/math.hpp"
#include "../base/assert.hpp"

#include "../std/limits.hpp"

namespace df
{
  double VizualizationParams::GetVisualScale()
  {
    ASSERT(!my::AlmostEqual(m_visualScale, numeric_limits<double>::min()), ());
    return m_visualScale;
  }

  void VizualizationParams::SetVisualScale(double visualScale)
  {
    m_visualScale = visualScale;
  }

  double VizualizationParams::m_visualScale = numeric_limits<double>::min();
}
