#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/apply_feature_functors.hpp"

#include "indexer/drawing_rules.hpp"

#include "base/assert.hpp"

namespace df
{
dp::Color GetColorConstant(ColorConstant const & constant)
{
  uint32_t const color = drule::rules().GetColor(constant);
  return ToDrapeColor(color);
}
} // namespace df
