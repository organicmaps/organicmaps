#include "sailfish/helpers.hpp"

#include "map/bookmark_manager.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "indexer/categories_holder.hpp"

#include "platform/distance.hpp"
#include "platform/duration.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"

#include <QGuiApplication>
#include <QInputMethod>
#include <QLocale>
#include <QTime>

#include <algorithm>
#include <iterator>

#include <MDConfItem>

namespace sailfish
{
QString Localized(QString const & key, QStringList const & args)
{
  QString s = QString::fromStdString(platform::GetLocalizedString(key.toStdString()));
  for (int i = 0; i < args.size(); ++i)
  {
    s.replace(QStringLiteral("%%1$@").arg(i + 1), args[i]);
    s.replace(QStringLiteral("%%1$d").arg(i + 1), args[i]);
  }
  if (!args.isEmpty())
    s.replace(QStringLiteral("%@"), args[0]).replace(QStringLiteral("%d"), args[0]);
  return s;
}

QString ToQString(std::string_view s)
{
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

std::string GetInputLocale()
{
  std::string locale = QGuiApplication::inputMethod()->locale().name().replace('_', '-').toStdString();
  if (CategoriesHolder::MapLocaleToInteger(locale) == CategoriesHolder::kUnsupportedLocaleCode)
  {
    locale = locale.substr(0, locale.find('-'));
    if (CategoriesHolder::MapLocaleToInteger(locale) == CategoriesHolder::kUnsupportedLocaleCode)
      locale = "en";
  }
  return locale;
}

std::vector<StringUtf8Multilang::Lang> SortedLanguages()
{
  auto const & supported = StringUtf8Multilang::GetSupportedLanguages(false /* includeServiceLangs */);
  std::vector<StringUtf8Multilang::Lang> languages(supported.begin(), supported.end());
  std::sort(languages.begin(), languages.end(), [](auto const & a, auto const & b) { return a.m_name < b.m_name; });
  return languages;
}

QString ColorName(dp::Color const & color)
{
  return QStringLiteral("#%1").arg(color.GetRGBA() >> 8, 6, 16, QLatin1Char('0'));
}

QStringList PresetColors()
{
  QStringList colors;
  for (auto const preset : kml::kOrderedPredefinedColors)
    colors.append(ColorName(kml::ColorFromPredefinedColor(preset)));
  return colors;
}

dp::Color PresetColor(int index)
{
  auto const & presets = kml::kOrderedPredefinedColors;
  ASSERT(index >= 0 && index < static_cast<int>(presets.size()), (index));
  return kml::ColorFromPredefinedColor(presets[static_cast<size_t>(index)]);
}

int PresetIndex(dp::Color const & color)
{
  auto const & presets = kml::kOrderedPredefinedColors;
  auto const it = std::find_if(presets.begin(), presets.end(), [&color](kml::PredefinedColor preset)
  { return kml::ColorFromPredefinedColor(preset).GetRGBA() == color.GetRGBA(); });
  return it != presets.end() ? static_cast<int>(std::distance(presets.begin(), it)) : -1;
}

bool IsSavedUserItem(BookmarkManager const & manager, uint64_t id, bool isTrack)
{
  if (isTrack)
    return id != kml::kInvalidTrackId && id != kml::kTempRelationTrackId && manager.HasTrack(id);
  return BookmarkManager::IsBookmark(id) && manager.HasBookmark(id);
}

QString FormatDuration(long seconds)
{
  return QString::fromStdString(platform::Duration(static_cast<unsigned long>(seconds)).GetHoursMinutesString());
}

bool Is24HourClock()
{
  // Never destroyed: dconf may be gone at exit.
  static auto const * timeFormat = new MDConfItem(QStringLiteral("/sailfish/i18n/lc_timeformat24h"));
  QString const setting = timeFormat->value().toString();
  if (setting.isEmpty())
    return !QLocale::system().timeFormat(QLocale::ShortFormat).contains("ap", Qt::CaseInsensitive);
  return setting == "24";
}

QString FormatTime(QTime const & time)
{
  return QLocale::system().toString(time, Is24HourClock() ? QStringLiteral("HH:mm") : QStringLiteral("h:mm AP"));
}

QString FormatSize(qint64 bytes)
{
  qint64 constexpr kMb = 1024 * 1024;
  qint64 constexpr kGb = 1024 * kMb;
  if (bytes < kGb)
    return QString::number(std::max<qint64>(1, (bytes + kMb / 2) / kMb)) + ' ' + Localized("mb");
  return QLocale::system().toString(static_cast<double>(bytes) / kGb, 'f', 1) + ' ' + Localized("gb");
}

QString FormatSpeed(double metersPerSecond)
{
  auto const units = measurement_utils::GetMeasurementUnits();
  return QString::fromStdString(measurement_utils::FormatSpeedNumeric(metersPerSecond, units) + " " +
                                platform::GetLocalizedSpeedUnits(units));
}

QString FormatAltitude(double meters)
{
  return QStringLiteral("▲") + QString::fromStdString(platform::Distance::FormatAltitude(meters));
}

QString FormatLatLon(double lat, double lon)
{
  return QString::fromStdString(measurement_utils::FormatLatLon(lat, lon, true /* withComma */, 5));
}

QString JoinDetails(QStringList const & parts)
{
  return parts.join(QStringLiteral(" • "));
}

MapStyle BaseMapStyle(bool dark, bool outdoors)
{
  if (outdoors)
    return dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
}
}  // namespace sailfish
