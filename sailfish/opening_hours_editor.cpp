#include "sailfish/opening_hours_editor.hpp"

#include "sailfish/helpers.hpp"

#include "editor/opening_hours_ui.hpp"

#include <QLocale>
#include <QStringList>
#include <QTime>

namespace sailfish
{
namespace
{
using editor::ui::TimeTable;
using osmoh::HourMinutes;

QString FormatHourMinutes(HourMinutes const & hm, bool namedNoonMidnight)
{
  // 12:00 AM and PM are often misread; a 24-hour clock has no such doubt.
  auto const hours = hm.GetHoursCount();
  if (namedNoonMidnight && hm.GetMinutesCount() == 0 && !Is24HourClock())
  {
    if (hours == 12)
      return Localized("noon");
    if (hours == 0 || hours == 24)
      return Localized("midnight");
  }
  // 24:00 closes at the end of the day; QTime can't hold it.
  if (hours == 24 && hm.GetMinutesCount() == 0)
    return QStringLiteral("24:00");
  return FormatTime(QTime(static_cast<int>(hours % 24), static_cast<int>(hm.GetMinutesCount())));
}
}  // namespace

QString ShortDayName(int day)
{
  auto name = QLocale::system().dayName(day == 1 ? 7 : day - 1, QLocale::ShortFormat);
  if (!name.isEmpty())
    name[0] = name[0].toUpper();
  return name;
}

QString FormatShifts(TimeTable const & tt, QString const & separator, bool namedNoonMidnight)
{
  QStringList shifts;
  auto const add = [&](HourMinutes const & start, HourMinutes const & end)
  {
    // A start after the end is an overnight shift; only empty ones are dropped.
    if (start.GetDurationCount() != end.GetDurationCount())
      shifts.append(FormatHourMinutes(start, namedNoonMidnight) + "—" + FormatHourMinutes(end, namedNoonMidnight));
  };
  auto start = tt.GetOpeningTime().GetStart().GetHourMinutes();
  for (auto const & closed : tt.GetExcludeTime())
  {
    add(start, closed.GetStart().GetHourMinutes());
    start = closed.GetEnd().GetHourMinutes();
  }
  add(start, tt.GetOpeningTime().GetEnd().GetHourMinutes());
  return shifts.join(separator);
}
}  // namespace sailfish
