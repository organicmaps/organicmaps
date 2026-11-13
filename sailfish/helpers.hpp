#pragma once

#include "indexer/map_style.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/settings.hpp"

#include <QString>
#include <QStringList>

#include <string>
#include <string_view>
#include <vector>

class QTime;

namespace sailfish
{
template <typename T>
T LoadSetting(std::string_view key, T defaultValue)
{
  T value;
  return settings::Get(key, value) ? value : defaultValue;
}

// A shared UI string from data/strings, with its placeholders (%@, %1$@, %d, %1$d) filled in.
QString Localized(QString const & key, QStringList const & args = {});
QString ToQString(std::string_view s);
// The search language follows the keyboard, or English if it has no categories.
std::string GetInputLocale();
// Sorted by their native names.
std::vector<StringUtf8Multilang::Lang> SortedLanguages();

QString FormatDuration(long seconds);
// Sailfish sets the clock format apart from the language.
bool Is24HourClock();
QString FormatTime(QTime const & time);
QString FormatSize(qint64 bytes);
QString FormatSpeed(double metersPerSecond);
QString FormatAltitude(double meters);
// 5 decimals without trailing zeros.
QString FormatLatLon(double lat, double lon);
// The parts of one line of details, like "5 km • 10 min".
QString JoinDetails(QStringList const & parts);
// For StyledText from files and other users: HTML without images, which would load by themselves, or escaped plain
// text with its line breaks.
QString UserStyledText(std::string const & text);

// Without the vehicle style used while navigating.
MapStyle BaseMapStyle(bool dark, bool outdoors);
}  // namespace sailfish
