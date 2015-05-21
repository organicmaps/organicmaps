#pragma once

#include "navigator.hpp"

#include "render/scales_processor.hpp"

#include "geometry/any_rect2d.hpp"

namespace navi
{

m2::AnyRectD ToRotated(m2::RectD const & rect, Navigator const & navigator);
void SetRectFixedAR(m2::AnyRectD const & rect, ScalesProcessor const & scales, Navigator & navigator);

} // namespace navi
