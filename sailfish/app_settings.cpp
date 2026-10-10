#include "sailfish/app_settings.hpp"

#include "sailfish/file_log.hpp"
#include "sailfish/helpers.hpp"

#include "map/framework.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <QDesktopServices>
#include <QFileInfo>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QUrl>
#include <QVariantMap>

namespace sailfish
{
namespace
{
std::string_view constexpr kMapAppearance = "SailfishMapAppearance";
std::string_view constexpr kZoomButtons = "SailfishZoomButtons";
std::string_view constexpr kKeepScreenOn = "SailfishKeepScreenOn";
std::string_view constexpr kLogging = "SailfishFileLogging";
std::string_view constexpr kEditsPublicNoticeShown = "SailfishEditsPublicNoticeShown";
// The Android key, without the Sailfish prefix.
std::string_view constexpr kAutoNightInNavigation = "AutoDarkNavigation";
std::string_view constexpr kSearchHistory = "SailfishSearchHistory";
std::string_view constexpr kAutoDownload = "SailfishAutoDownload";
std::string_view constexpr kMobileData = "SailfishMobileData";

struct Mode3d
{
  bool m_allow3d;
  bool m_buildings;
};

QNetworkConfiguration DefaultNetwork()
{
  return QNetworkConfigurationManager().defaultConfiguration();
}

bool IsOnMobileData()
{
  switch (DefaultNetwork().bearerTypeFamily())
  {
  case QNetworkConfiguration::Bearer2G:
  case QNetworkConfiguration::Bearer3G:
  case QNetworkConfiguration::Bearer4G: return true;
  default: return false;
  }
}

static_assert(AppSettings::PowerEconomyMaximum == static_cast<int>(power_management::Scheme::EconomyMaximum));
static_assert(AppSettings::PowerAuto == static_cast<int>(power_management::Scheme::Auto));

Mode3d Load3dMode()
{
  Mode3d mode;
  Framework::Load3dMode(mode.m_allow3d, mode.m_buildings);
  return mode;
}
}  // namespace

AppSettings::AppSettings(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework) {}

template <class T>
void AppSettings::SaveUiSetting(std::string_view key, T value)
{
  settings::Set(key, value);
  emit changed();
}

void AppSettings::Set3dMode(bool allow3d, bool buildings)
{
  Framework::Save3dMode(allow3d, buildings);
  m_framework.Allow3dMode(allow3d, buildings);
  emit changed();
}

int AppSettings::mapAppearance() const
{
  return LoadSetting<int>(kMapAppearance, AppearanceAuto);
}

void AppSettings::setMapAppearance(int appearance)
{
  SaveUiSetting(kMapAppearance, appearance);
}

void AppSettings::applyMapAppearance(bool dark)
{
  MapStyle const current = m_framework.GetMapStyle();
  MapStyle const style = dark ? GetDarkMapStyleVariant(current) : GetLightMapStyleVariant(current);
  if (style != current)
    m_framework.SetMapStyle(style);
}

int AppSettings::units() const
{
  return static_cast<int>(measurement_utils::GetMeasurementUnits());
}

void AppSettings::setUnits(int units)
{
  settings::Set(settings::kMeasurementUnits, static_cast<measurement_utils::Units>(units));
  m_framework.SetupMeasurementSystem();
  emit changed();
}

bool AppSettings::zoomButtons() const
{
  return LoadSetting(kZoomButtons, true);
}

void AppSettings::setZoomButtons(bool enabled)
{
  SaveUiSetting(kZoomButtons, enabled);
}

bool AppSettings::showDownloadedRegions() const
{
  return m_framework.IsShowDownloadedRegions();
}

void AppSettings::setShowDownloadedRegions(bool enabled)
{
  m_framework.SetShowDownloadedRegions(enabled);
  emit changed();
}

bool AppSettings::largeFonts() const
{
  return m_framework.LoadLargeFontsSize();
}

void AppSettings::setLargeFonts(bool enabled)
{
  m_framework.SetLargeFontsSize(enabled);
  emit changed();
}

bool AppSettings::transliteration() const
{
  return Framework::LoadTransliteration();
}

void AppSettings::setTransliteration(bool enabled)
{
  Framework::SaveTransliteration(enabled);
  m_framework.AllowTransliteration(enabled);
  emit changed();
}

bool AppSettings::keepScreenOn() const
{
  return LoadSetting(kKeepScreenOn, false);
}

void AppSettings::setKeepScreenOn(bool enabled)
{
  SaveUiSetting(kKeepScreenOn, enabled);
}

// static
bool AppSettings::IsSearchHistoryEnabled()
{
  return LoadSetting(kSearchHistory, true);
}

bool AppSettings::searchHistory() const
{
  return IsSearchHistoryEnabled();
}

void AppSettings::setSearchHistory(bool enabled)
{
  // Turning the history off forgets it.
  if (!enabled)
    m_framework.GetSearchAPI().ClearSearchHistory();
  SaveUiSetting(kSearchHistory, enabled);
}

QString AppSettings::mapLanguage() const
{
  return QString::fromStdString(Framework::GetMapLanguageCode());
}

void AppSettings::setMapLanguage(QString const & code)
{
  m_framework.SetMapLanguageCode(code.toStdString());
  emit changed();
}

QString AppSettings::mapLanguageName() const
{
  auto const code = Framework::GetMapLanguageCode();
  auto const name = StringUtf8Multilang::GetLangNameByCode(StringUtf8Multilang::GetLangIndex(code));
  return name.empty() ? QString::fromStdString(code) : ToQString(name);
}

QVariantList AppSettings::mapLanguages() const
{
  QVariantList languages;
  for (auto const & lang : SortedLanguages())
  {
    QVariantMap item;
    item["code"] = ToQString(lang.m_code);
    item["name"] = ToQString(lang.m_name);
    languages.append(item);
  }
  return languages;
}

bool AppSettings::buildings3d() const
{
  return Load3dMode().m_buildings;
}

void AppSettings::setBuildings3d(bool enabled)
{
  Set3dMode(Load3dMode().m_allow3d, enabled);
}

bool AppSettings::perspectiveView() const
{
  return Load3dMode().m_allow3d;
}

void AppSettings::setPerspectiveView(bool enabled)
{
  Set3dMode(enabled, Load3dMode().m_buildings);
}

bool AppSettings::editsPublicNoticeShown() const
{
  return LoadSetting(kEditsPublicNoticeShown, false);
}

void AppSettings::setEditsPublicNoticeShown(bool shown)
{
  SaveUiSetting(kEditsPublicNoticeShown, shown);
}

bool AppSettings::logging() const
{
  return file_log::IsEnabled();
}

void AppSettings::setLogging(bool enabled)
{
  file_log::Enable(enabled);
  SaveUiSetting(kLogging, enabled);
}

QString AppSettings::logUrl() const
{
  return QUrl::fromLocalFile(file_log::Path()).toString();
}

qint64 AppSettings::logSize() const
{
  return QFileInfo(file_log::Path()).size();
}

// static
void AppSettings::InitLogging()
{
  file_log::Enable(LoadSetting(kLogging, false));
}

bool AppSettings::autoNightInNavigation() const
{
  return LoadSetting(kAutoNightInNavigation, false);
}

void AppSettings::setAutoNightInNavigation(bool enabled)
{
  SaveUiSetting(kAutoNightInNavigation, enabled);
}

bool AppSettings::autoZoom() const
{
  return m_framework.LoadAutoZoom();
}

void AppSettings::setAutoZoom(bool enabled)
{
  m_framework.AllowAutoZoom(enabled);
  m_framework.SaveAutoZoom(enabled);
  emit changed();
}

// static
bool AppSettings::IsAutoDownloadEnabled()
{
  return LoadSetting(kAutoDownload, true);
}

bool AppSettings::IsOnWifi()
{
  return DefaultNetwork().bearerType() == QNetworkConfiguration::BearerWLAN;
}

void AppSettings::setAutoDownload(bool enabled)
{
  SaveUiSetting(kAutoDownload, enabled);
}

int AppSettings::mobileData() const
{
  return LoadSetting(kMobileData, static_cast<int>(MobileDataAsk));
}

void AppSettings::setMobileData(int mobileData)
{
  SaveUiSetting(kMobileData, mobileData);
}

int AppSettings::downloadPermission() const
{
  if (!IsOnMobileData())
    return DownloadAllowed;
  switch (mobileData())
  {
  case MobileDataAlways: return DownloadAllowed;
  case MobileDataNever: return DownloadDenied;
  default: return DownloadAsk;
  }
}

int AppSettings::powerScheme() const
{
  return static_cast<int>(m_framework.GetPowerManager().GetScheme());
}

void AppSettings::setPowerScheme(int scheme)
{
  m_framework.GetPowerManager().SetScheme(static_cast<power_management::Scheme>(scheme));
  emit changed();
}

int AppSettings::speedCamerasMode() const
{
  return static_cast<int>(m_framework.GetRoutingManager().GetSpeedCamManager().GetMode());
}

void AppSettings::setSpeedCamerasMode(int mode)
{
  m_framework.GetRoutingManager().GetSpeedCamManager().SetMode(static_cast<routing::SpeedCameraManagerMode>(mode));
  emit changed();
}

int AppSettings::bookmarksTextPlacement() const
{
  return static_cast<int>(Framework::GetBookmarksTextPlacement());
}

void AppSettings::setBookmarksTextPlacement(int placement)
{
  m_framework.SetBookmarksTextPlacement(static_cast<settings::Placement>(placement));
  emit changed();
}

bool AppSettings::bgTilesEnabled() const
{
  return Framework::IsBackgroundTilesEnabled();
}

QString AppSettings::bgTilesUrl() const
{
  return QString::fromStdString(Framework::GetBackgroundTilesURL());
}

int AppSettings::bgTilesCacheSize() const
{
  return static_cast<int>(Framework::GetBackgroundTilesCacheSize());
}

int AppSettings::bgTilesOpacity() const
{
  return static_cast<int>(Framework::GetBackgroundTilesAreaOpacity());
}

void AppSettings::setBackgroundTiles(bool enabled, QString const & url, int cacheSizeMb, int opacityPct)
{
  m_framework.SetBackgroundTiles(enabled, url.trimmed().toStdString(), static_cast<uint32_t>(cacheSizeMb),
                                 static_cast<uint32_t>(opacityPct));
  emit changed();
}

bool AppSettings::isWellFormedTilesUrl(QString const & url) const
{
  return Framework::IsWellFormedBackgroundTilesURL(url.trimmed().toStdString());
}

QString AppSettings::donateUrl() const
{
  return QString::fromStdString(m_framework.GetDonateUrl());
}

int AppSettings::helpPromo() const
{
  if (m_framework.GetDonateUrl().empty())
    return PromoNone;
  if (m_framework.CanShowCrowdfundingPromo())
    return PromoCrowdfunding;
  return LoadSetting(settings::kNY, false) ? PromoNewYear : PromoNone;
}

void AppSettings::openDonatePage()
{
  QDesktopServices::openUrl(QUrl(donateUrl()));
  m_framework.DidShowDonationPage();
  emit changed();
}
}  // namespace sailfish
