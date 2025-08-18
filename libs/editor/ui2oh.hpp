#pragma once

#include "editor/opening_hours_ui.hpp"

namespace osmoh
{
class OpeningHours;
}  // namespace osmoh

namespace editor
{
osmoh::OpeningHours MakeOpeningHours(ui::TimeTableSet const & tts);
bool MakeTimeTableSet(osmoh::OpeningHours const & oh, ui::TimeTableSet & tts);
}  // namespace editor
