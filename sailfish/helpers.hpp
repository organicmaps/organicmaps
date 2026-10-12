#pragma once

#include "indexer/map_style.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/settings.hpp"

#include <QString>
#include <QStringList>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class BookmarkManager;
class QTime;

namespace dp
{
struct Color;
}  // namespace dp

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

// "#rrggbb" for QML.
QString ColorName(dp::Color const & color);
// In the order of the shared color pickers.
QStringList PresetColors();
dp::Color PresetColor(int index);
// The index of a preset color, -1 for another color.
int PresetIndex(dp::Color const & color);
// Not deleted, nor the temporary track of a route relation, which the core can't edit or delete.
bool IsSavedUserItem(BookmarkManager const & manager, uint64_t id, bool isTrack);

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

// Without the vehicle style used while navigating.
MapStyle BaseMapStyle(bool dark, bool outdoors);
}  // namespace sailfish
