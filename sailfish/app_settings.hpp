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
  Q_PROPERTY(QString mapLanguage READ mapLanguage WRITE setMapLanguage NOTIFY changed)
  Q_PROPERTY(QString mapLanguageName READ mapLanguageName NOTIFY changed)
  // Map languages as {code, name}.
  Q_PROPERTY(QVariantList mapLanguages READ mapLanguages CONSTANT)
  Q_PROPERTY(QString donateUrl READ donateUrl CONSTANT)
  // The Promo the help button shows instead of the logo.
  Q_PROPERTY(int helpPromo READ helpPromo NOTIFY changed)
  Q_PROPERTY(bool buildings3d READ buildings3d WRITE setBuildings3d NOTIFY changed)
  // The file log that "Report a bug" shares.
  Q_PROPERTY(bool logging READ logging WRITE setLogging NOTIFY changed)
  Q_PROPERTY(QString logUrl READ logUrl CONSTANT)
  // PowerScheme.
  Q_PROPERTY(int powerScheme READ powerScheme WRITE setPowerScheme NOTIFY changed)
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
    AppearanceDark
  };
  Q_ENUM(MapAppearance)

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
  QString mapLanguage() const;
  void setMapLanguage(QString const & code);
  QString mapLanguageName() const;
  QVariantList mapLanguages() const;
  QString donateUrl() const;
  int helpPromo() const;
  bool buildings3d() const;
  void setBuildings3d(bool enabled);
  bool logging() const;
  void setLogging(bool enabled);
  QString logUrl() const;
  int powerScheme() const;
  void setPowerScheme(int scheme);
  bool bgTilesEnabled() const;
  QString bgTilesUrl() const;
  int bgTilesCacheSize() const;
  int bgTilesOpacity() const;

  // Opens the donation page, which ends the crowdfunding promo.
  Q_INVOKABLE void openDonatePage();
  // Read when shown: the file grows without a change signal.
  Q_INVOKABLE qint64 logSize() const;
  Q_INVOKABLE void setBackgroundTiles(bool enabled, QString const & url, int cacheSizeMb, int opacityPct);
  Q_INVOKABLE bool isWellFormedTilesUrl(QString const & url) const;
  Q_INVOKABLE void applyMapAppearance(bool dark);

  static void InitLogging();

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
