#include "editor/ui2oh.hpp"

#include "std/array.hpp"
#include "std/string.hpp"

#include "3party/opening_hours/opening_hours.hpp"

namespace editor
{
osmoh::OpeningHours ConvertOpeningHours(ui::TimeTableSet const & tt)
{
  return string(); // TODO(mgsergio): // Just a dummy.
}

ui::TimeTableSet ConvertOpeningHours(osmoh::OpeningHours const & oh)
{
  return {};
}
} // namespace editor
