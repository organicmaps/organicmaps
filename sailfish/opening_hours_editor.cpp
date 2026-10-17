#include "sailfish/opening_hours_editor.hpp"

#include "sailfish/helpers.hpp"

#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

#include "base/assert.hpp"

#include <QLocale>
#include <QStringList>
#include <QTime>
#include <QVariantMap>

#include <sstream>

namespace sailfish
{
namespace
{
using editor::ui::TimeTable;
using editor::ui::TimeTableSet;
using osmoh::HourMinutes;

int Minutes(osmoh::Time const & time)
{
  return static_cast<int>(time.GetHourMinutes().GetDurationCount());
}

osmoh::Timespan Span(int start, int end)
{
  return {HourMinutes::TMinutes(start), HourMinutes::TMinutes(end)};
}

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

QString Uncapitalize(QString text)
{
  if (!text.isEmpty())
    text[0] = text[0].toLower();
  return text;
}

// E.g. "Mon-Wed, Fri".
QString FormatDays(TimeTable const & tt)
{
  std::vector<int> days;
  for (auto const day : tt.GetOpeningDays())
    days.push_back(static_cast<int>(day));
  QString text;
  for (size_t i = 0; i < days.size();)
  {
    size_t last = i;
    while (last + 1 < days.size() && days[last + 1] == days[last] + 1)
      ++last;
    if (!text.isEmpty())
      text += ", ";
    text += ShortDayName(days[i]);
    if (last != i)
      text += "-" + ShortDayName(days[last]);
    i = last + 1;
  }
  return text;
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

QString FormatOpeningHours(QString const & value)
{
  TimeTableSet timetables;
  if (!editor::MakeTimeTableSet(osmoh::OpeningHours(value.trimmed().toStdString()), timetables))
    return value;
  auto const openTime = [](TimeTable const & tt, QString const & allDay)
  {
    if (tt.IsTwentyFourHours())
      return allDay;
    auto const shifts = FormatShifts(tt, ", ", false /* namedNoonMidnight */);
    // A working day fully covered by breaks has no open shift.
    return shifts.isEmpty() ? Uncapitalize(Localized("day_off")) : shifts;
  };
  auto const & first = *timetables.begin();
  if (first.GetOpeningDays().size() == 7)
  {
    if (first.IsTwentyFourHours())
      return Localized("twentyfour_seven");
    return Localized("daily") + " " + openTime(first, {});
  }
  QStringList lines;
  for (auto const & tt : timetables)
    lines.append(FormatDays(tt) + " " + openTime(tt, Uncapitalize(Localized("editor_time_allday"))));
  return lines.join('\n');
}

bool IsValidOpeningHours(QString const & value)
{
  auto const text = value.trimmed();
  return text.isEmpty() || osmoh::OpeningHours(text.toStdString()).IsValid();
}

OpeningHoursEditor::OpeningHoursEditor(QObject * parent)
  : QObject(parent)
  , m_timetables(std::make_unique<TimeTableSet>())
{}

OpeningHoursEditor::~OpeningHoursEditor() = default;

QString OpeningHoursEditor::value() const
{
  if (!m_simple)
    return m_text;
  std::ostringstream rule;
  rule << editor::MakeOpeningHours(*m_timetables).GetRule();
  return QString::fromStdString(rule.str());
}

void OpeningHoursEditor::setValue(QString const & value)
{
  auto const text = value.trimmed();
  TimeTableSet timetables;
  // Empty starts from the default schedule.
  m_simple = text.isEmpty() || editor::MakeTimeTableSet(osmoh::OpeningHours(text.toStdString()), timetables);
  *m_timetables = std::move(timetables);
  m_text = text;
  emit changed();
}

QVariantList OpeningHoursEditor::timetables() const
{
  QVariantList result;
  for (auto const & tt : *m_timetables)
  {
    QVariantList days;
    for (auto const day : tt.GetOpeningDays())
      days.append(static_cast<int>(day));
    QVariantList closed;
    for (auto const & span : tt.GetExcludeTime())
      closed.append(QVariantMap{{"start", Minutes(span.GetStart())}, {"end", Minutes(span.GetEnd())}});
    auto const & opening = tt.GetOpeningTime();
    result.append(QVariantMap{{"days", days},
                              {"allDay", tt.IsTwentyFourHours()},
                              {"open", Minutes(opening.GetStart())},
                              {"close", Minutes(opening.GetEnd())},
                              {"closed", closed},
                              {"canAddClosed", !tt.IsTwentyFourHours() && tt.CanAddExcludeTime()}});
  }
  return result;
}

bool OpeningHoursEditor::canAddTimetable() const
{
  return !m_timetables->GetUnhandledDays().empty();
}

void OpeningHoursEditor::Edit(int index, std::function<bool(TimeTable &)> const & change)
{
  ASSERT(index >= 0 && static_cast<size_t>(index) < m_timetables->Size(), (index));
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  // Commit is refused when the change would break another schedule, e.g. take its last day.
  if (change(tt))
    tt.Commit();
  emit changed();
}

void OpeningHoursEditor::setDay(int index, int day, bool on)
{
  // A schedule keeps at least one day; a day taken here leaves the other schedules.
  Edit(index, [weekday = static_cast<osmoh::Weekday>(day), on](TimeTable & tt)
  {
    if (!on)
      return tt.RemoveWorkingDay(weekday);
    tt.AddWorkingDay(weekday);
    return true;
  });
}

void OpeningHoursEditor::setAllDay(int index, bool on)
{
  Edit(index, [on](TimeTable & tt)
  {
    tt.SetTwentyFourHours(on);
    return true;
  });
}

void OpeningHoursEditor::setOpeningTime(int index, int open, int close)
{
  Edit(index, [open, close](TimeTable & tt) { return tt.SetOpeningTime(Span(open, close)); });
}

void OpeningHoursEditor::addClosed(int index)
{
  Edit(index, [](TimeTable & tt) { return tt.AddExcludeTime(tt.GetPredefinedExcludeTime()); });
}

void OpeningHoursEditor::setClosed(int index, int closedIndex, int start, int end)
{
  // Rejected when outside the opening time; the view shows the kept value again.
  ASSERT_GREATER_OR_EQUAL(closedIndex, 0, ());
  Edit(index, [closedIndex, start, end](TimeTable & tt)
  { return tt.ReplaceExcludeTime(Span(start, end), static_cast<size_t>(closedIndex)); });
}

void OpeningHoursEditor::removeClosed(int index, int closedIndex)
{
  ASSERT_GREATER_OR_EQUAL(closedIndex, 0, ());
  Edit(index, [closedIndex](TimeTable & tt) { return tt.RemoveExcludeTime(static_cast<size_t>(closedIndex)); });
}

void OpeningHoursEditor::addTimetable()
{
  if (m_timetables->Append(m_timetables->GetComplementTimeTable()))
    emit changed();
}

void OpeningHoursEditor::removeTimetable(int index)
{
  ASSERT_GREATER_OR_EQUAL(index, 0, ());
  if (m_timetables->Remove(static_cast<size_t>(index)))
    emit changed();
}
}  // namespace sailfish
