#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <string_view>

class Framework;

namespace sailfish
{
// Core settings go through the Framework; UI only ones are kept in the same settings file.
class AppSettings : public QObject
{
  Q_OBJECT
  // MapAppearance; Auto follows the Sailfish ambience.
  Q_PROPERTY(int mapAppearance READ mapAppearance WRITE setMapAppearance NOTIFY changed)
  // measurement_utils::Units: 0 metric, 1 imperial.
  Q_PROPERTY(int units READ units WRITE setUnits NOTIFY changed)
  Q_PROPERTY(bool zoomButtons READ zoomButtons WRITE setZoomButtons NOTIFY changed)
  Q_PROPERTY(bool showDownloadedRegions READ showDownloadedRegions WRITE setShowDownloadedRegions NOTIFY changed)
  Q_PROPERTY(bool largeFonts READ largeFonts WRITE setLargeFonts NOTIFY changed)
  Q_PROPERTY(bool transliteration READ transliteration WRITE setTransliteration NOTIFY changed)
  Q_PROPERTY(bool keepScreenOn READ keepScreenOn WRITE setKeepScreenOn NOTIFY changed)
  Q_PROPERTY(bool searchHistory READ searchHistory WRITE setSearchHistory NOTIFY changed)
  Q_PROPERTY(QString mapLanguage READ mapLanguage WRITE setMapLanguage NOTIFY changed)
  Q_PROPERTY(QString mapLanguageName READ mapLanguageName NOTIFY changed)
  // Map languages as {code, name}.
  Q_PROPERTY(QVariantList mapLanguages READ mapLanguages CONSTANT)
  Q_PROPERTY(QString donateUrl READ donateUrl CONSTANT)
  // The Promo the help button shows instead of the logo.
  Q_PROPERTY(int helpPromo READ helpPromo NOTIFY changed)
  Q_PROPERTY(bool buildings3d READ buildings3d WRITE setBuildings3d NOTIFY changed)
  Q_PROPERTY(bool autoDownload READ autoDownload WRITE setAutoDownload NOTIFY changed)
  // MobileData: map downloads over a cellular connection.
  Q_PROPERTY(int mobileData READ mobileData WRITE setMobileData NOTIFY changed)
  Q_PROPERTY(bool perspectiveView READ perspectiveView WRITE setPerspectiveView NOTIFY changed)
  Q_PROPERTY(bool autoZoom READ autoZoom WRITE setAutoZoom NOTIFY changed)
  // Dark map while navigating at night, see Routing::darkOutside.
  Q_PROPERTY(bool autoNightInNavigation READ autoNightInNavigation WRITE setAutoNightInNavigation NOTIFY changed)
  // The file log that "Report a bug" shares.
  Q_PROPERTY(bool logging READ logging WRITE setLogging NOTIFY changed)
  Q_PROPERTY(QString logUrl READ logUrl CONSTANT)
  Q_PROPERTY(bool editsPublicNoticeShown READ editsPublicNoticeShown WRITE setEditsPublicNoticeShown NOTIFY changed)
  // PowerScheme.
  Q_PROPERTY(int powerScheme READ powerScheme WRITE setPowerScheme NOTIFY changed)
  // routing::SpeedCameraManagerMode.
  Q_PROPERTY(int speedCamerasMode READ speedCamerasMode WRITE setSpeedCamerasMode NOTIFY changed)
  // settings::Placement.
  Q_PROPERTY(int bookmarksTextPlacement READ bookmarksTextPlacement WRITE setBookmarksTextPlacement NOTIFY changed)
  // Imagery from a user provided XYZ tile server.
  Q_PROPERTY(bool bgTilesEnabled READ bgTilesEnabled NOTIFY changed)
  Q_PROPERTY(QString bgTilesUrl READ bgTilesUrl NOTIFY changed)
  Q_PROPERTY(int bgTilesCacheSize READ bgTilesCacheSize NOTIFY changed)
  Q_PROPERTY(int bgTilesOpacity READ bgTilesOpacity NOTIFY changed)

public:
  enum MapAppearance
  {
    AppearanceAuto,
    AppearanceLight,
    AppearanceDark,
    // Light from dawn till dusk.
    AppearanceScheduled
  };
  Q_ENUM(MapAppearance)

  enum MobileData
  {
    MobileDataAsk,
    MobileDataAlways,
    MobileDataNever
  };
  Q_ENUM(MobileData)

  enum Promo
  {
    PromoNone,
    // A donation campaign runs and the donation page wasn't opened during it.
    PromoCrowdfunding,
    // The New Year period, set by the server.
    PromoNewYear
  };
  Q_ENUM(Promo)

  // Mirrors power_management::Scheme.
  enum PowerScheme
  {
    PowerNone,
    PowerNormal,
    PowerEconomyMedium,
    PowerEconomyMaximum,
    PowerAuto
  };
  Q_ENUM(PowerScheme)

  enum DownloadPermission
  {
    DownloadAllowed,
    DownloadAsk,
    DownloadDenied
  };
  Q_ENUM(DownloadPermission)

  explicit AppSettings(Framework & framework, QObject * parent = nullptr);

  int mapAppearance() const;
  void setMapAppearance(int appearance);
  int units() const;
  void setUnits(int units);
  bool zoomButtons() const;
  void setZoomButtons(bool enabled);
  bool showDownloadedRegions() const;
  void setShowDownloadedRegions(bool enabled);
  bool largeFonts() const;
  void setLargeFonts(bool enabled);
  bool transliteration() const;
  void setTransliteration(bool enabled);
  bool keepScreenOn() const;
  void setKeepScreenOn(bool enabled);
  bool searchHistory() const;
  void setSearchHistory(bool enabled);
  QString mapLanguage() const;
  void setMapLanguage(QString const & code);
  QString mapLanguageName() const;
  QVariantList mapLanguages() const;
  QString donateUrl() const;
  int helpPromo() const;
  bool buildings3d() const;
  void setBuildings3d(bool enabled);
  bool autoDownload() const { return IsAutoDownloadEnabled(); }
  void setAutoDownload(bool enabled);
  int mobileData() const;
  void setMobileData(int mobileData);
  bool perspectiveView() const;
  void setPerspectiveView(bool enabled);
  bool autoZoom() const;
  void setAutoZoom(bool enabled);
  bool autoNightInNavigation() const;
  void setAutoNightInNavigation(bool enabled);
  bool logging() const;
  void setLogging(bool enabled);
  QString logUrl() const;
  bool editsPublicNoticeShown() const;
  void setEditsPublicNoticeShown(bool shown);
  int powerScheme() const;
  void setPowerScheme(int scheme);
  int speedCamerasMode() const;
  void setSpeedCamerasMode(int mode);
  int bookmarksTextPlacement() const;
  void setBookmarksTextPlacement(int placement);
  bool bgTilesEnabled() const;
  QString bgTilesUrl() const;
  int bgTilesCacheSize() const;
  int bgTilesOpacity() const;

  // Opens the donation page, which ends the crowdfunding promo.
  Q_INVOKABLE void openDonatePage();
  // Read when shown: the file grows without a change signal.
  Q_INVOKABLE qint64 logSize() const;
  // The mobileData setting applies only to cellular connections.
  Q_INVOKABLE int downloadPermission() const;
  Q_INVOKABLE void setBackgroundTiles(bool enabled, QString const & url, int cacheSizeMb, int opacityPct);
  Q_INVOKABLE bool isWellFormedTilesUrl(QString const & url) const;
  Q_INVOKABLE void applyMapAppearance(bool dark);

  static void InitLogging();
  static bool IsSearchHistoryEnabled();
  static bool IsAutoDownloadEnabled();
  // Automatic map downloads only run on Wi-Fi.
  static bool IsOnWifi();

signals:
  void changed();

private:
  // Saves a UI only setting, next to the core ones.
  template <class T>
  void SaveUiSetting(std::string_view key, T value);
  void Set3dMode(bool allow3d, bool buildings);

  Framework & m_framework;
};
}  // namespace sailfish
