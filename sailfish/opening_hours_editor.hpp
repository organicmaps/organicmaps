#pragma once

#include <QString>

// Nested namespaces spelled out for the Qt 5.6 moc.
namespace editor
{
namespace ui
{
class TimeTable;
}  // namespace ui
}  // namespace editor

namespace sailfish
{
// A capitalized short weekday name; osmoh::Weekday counts from Sunday = 1.
QString ShortDayName(int day);
// The opening shifts of a day: its opening time without the closed times, joined by separator. With
// namedNoonMidnight, 12-hour clocks show noon and midnight by name.
QString FormatShifts(editor::ui::TimeTable const & tt, QString const & separator, bool namedNoonMidnight);
}  // namespace sailfish
