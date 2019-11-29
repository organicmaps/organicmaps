#pragma once

#include "editor/osm_editor.hpp"

#include "metrics/eye_info.hpp"

#include <optional>

namespace place_page
{
class Info;
}

class FeatureType;

namespace utils
{
eye::MapObject MakeEyeMapObject(place_page::Info const & info);
eye::MapObject MakeEyeMapObject(FeatureType & ft, osm::Editor const & editor);

void RegisterEyeEventIfPossible(eye::MapObject::Event::Type const type,
                                std::optional<m2::PointD> const & userPos,
                                place_page::Info const & info);
}  // namespace utils
