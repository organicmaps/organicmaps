#pragma once

#include "editor/opening_hours_ui.hpp"

#include "3party/opening_hours/opening_hours.hpp"


namespace editor
{
osmoh::OpeningHours ConvertOpeningHours(ui::TTimeTables const & tt);
ui::TTimeTables ConvertOpeningHours(osmoh::OpeningHours const & oh);
} // namespace editor
