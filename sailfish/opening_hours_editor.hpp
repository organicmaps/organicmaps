#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <functional>
#include <memory>

// Nested namespaces spelled out for the Qt 5.6 moc.
namespace editor
{
namespace ui
{
class TimeTable;
class TimeTableSet;
}  // namespace ui
}  // namespace editor

namespace sailfish
{
// Empty, which removes the opening hours, or a valid opening_hours value.
bool IsValidOpeningHours(QString const & value);
// A capitalized short weekday name; osmoh::Weekday counts from Sunday = 1.
QString ShortDayName(int day);
// The opening shifts of a day: its opening time without the closed times, joined by separator. With
// namedNoonMidnight, 12-hour clocks show noon and midnight by name.
QString FormatShifts(editor::ui::TimeTable const & tt, QString const & separator, bool namedNoonMidnight);
// A valid value as "24/7", "Daily 09:00—18:00" or a line per group of days, e.g. "Mon-Fri 09:00—17:00";
// the value itself when it isn't a simple schedule.
QString FormatOpeningHours(QString const & value);

// The core keeps the schedules valid: a day belongs to one schedule, and non-business hours lie within the
// opening time. Values that can't be shown as schedules are edited as text.
class OpeningHoursEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY changed)
  Q_PROPERTY(bool simple READ simple NOTIFY changed)
  // {days, allDay, open, close, closed, canAddClosed}: days are osmoh::Weekday values (Sunday is 1), times are
  // minutes since midnight and closed lists the non-business hours as {start, end}.
  Q_PROPERTY(QVariantList timetables READ timetables NOTIFY changed)
  Q_PROPERTY(bool canAddTimetable READ canAddTimetable NOTIFY changed)

public:
  explicit OpeningHoursEditor(QObject * parent = nullptr);
  ~OpeningHoursEditor() override;

  QString value() const;
  void setValue(QString const & value);
  bool simple() const { return m_simple; }
  QVariantList timetables() const;
  bool canAddTimetable() const;

  Q_INVOKABLE void setDay(int index, int day, bool on);
  Q_INVOKABLE void setAllDay(int index, bool on);
  Q_INVOKABLE void setOpeningTime(int index, int open, int close);
  Q_INVOKABLE void addClosed(int index);
  Q_INVOKABLE void setClosed(int index, int closedIndex, int start, int end);
  Q_INVOKABLE void removeClosed(int index, int closedIndex);
  // A schedule for the days left.
  Q_INVOKABLE void addTimetable();
  Q_INVOKABLE void removeTimetable(int index);
  // For text, see IsValidOpeningHours().
  Q_INVOKABLE bool isValid(QString const & value) const { return IsValidOpeningHours(value); }
  Q_INVOKABLE QString dayName(int day) const { return ShortDayName(day); }

signals:
  void changed();

private:
  // Commits when change returns true; emits changed() either way so the view resets rejected input.
  void Edit(int index, std::function<bool(editor::ui::TimeTable &)> const & change);

  std::unique_ptr<editor::ui::TimeTableSet> m_timetables;
  bool m_simple = true;
  QString m_text;
};
}  // namespace sailfish
