#pragma once

#include "editor/opening_hours_ui.hpp"

namespace osmoh
{
class OpeningHours;
} // namespace osmoh

namespace editor
{
osmoh::OpeningHours ConvertOpeningHours(ui::TimeTableSet const & tt);
ui::TimeTableSet ConvertOpeningHours(osmoh::OpeningHours const & oh);
} // namespace editor
