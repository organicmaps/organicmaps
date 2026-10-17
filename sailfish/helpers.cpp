#include "sailfish/helpers.hpp"

#include "indexer/categories_holder.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include <QGuiApplication>
#include <QInputMethod>
#include <QLocale>

#include <algorithm>

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

MapStyle BaseMapStyle(bool dark, bool outdoors)
{
  if (outdoors)
    return dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
}
}  // namespace sailfish
