#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <string>
#include <string_view>

class Framework;

namespace place_page
{
class Info;
}  // namespace place_page

namespace sailfish
{
// Kept free of map headers, which Qt 5.6 moc can't parse.
class PlacePage : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool open READ open NOTIFY changed)
  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString subtitle READ subtitle NOTIFY changed)
  Q_PROPERTY(QString address READ address NOTIFY changed)
  Q_PROPERTY(QString secondaryTitle READ secondaryTitle NOTIFY changed)
  Q_PROPERTY(QString osmDescription READ osmDescription NOTIFY changed)
  // See MissingMapInfo().
  Q_PROPERTY(QVariantMap country READ country NOTIFY countryChanged)
  Q_PROPERTY(QString apiBackUrl READ apiBackUrl NOTIFY changed)
  Q_PROPERTY(QString distance READ distance NOTIFY distanceChanged)
  // Degrees relative to the device heading, negative without a position.
  Q_PROPERTY(double azimuth READ azimuth NOTIFY distanceChanged)
  // From true north, like "123°".
  Q_PROPERTY(QString bearing READ bearing NOTIFY distanceChanged)
  Q_PROPERTY(QString shareText READ shareText NOTIFY changed)
  Q_PROPERTY(QString geoUri READ geoUri NOTIFY changed)
  Q_PROPERTY(QString coordinates READ coordinates NOTIFY changed)
  // Bare values of all available formats, for copying.
  Q_PROPERTY(QStringList coordinateValues READ coordinateValues NOTIFY changed)
  Q_PROPERTY(int openState READ openState NOTIFY changed)
  Q_PROPERTY(QString openTitle READ openTitle NOTIFY changed)
  Q_PROPERTY(QString openDescription READ openDescription NOTIFY changed)
  Q_PROPERTY(QString openingHours READ openingHours NOTIFY changed)
  // The week from today as {days, hours, today}; empty when it can't be shown as a table.
  Q_PROPERTY(QVariantList openingSchedule READ openingSchedule NOTIFY changed)
  Q_PROPERTY(QString wikiDescription READ wikiDescription NOTIFY changed)
  Q_PROPERTY(QString wikiUrl READ wikiUrl NOTIFY changed)
  // {icon, text, url} rows; url is opened externally when set.
  // {icon, text, url} rows; the icon is a theme image URL or a file in the icons folder.
  Q_PROPERTY(QVariantList details READ details NOTIFY changed)

public:
  enum OpenState
  {
    OpenUnknown,
    Open,
    Closed
  };
  Q_ENUM(OpenState)

  explicit PlacePage(Framework & framework, QObject * parent = nullptr);
  ~PlacePage() override;

  bool open() const { return m_open; }
  QString title() const { return m_title; }
  QString subtitle() const { return m_subtitle; }
  QString address() const { return m_address; }
  QString secondaryTitle() const { return m_secondaryTitle; }
  QString osmDescription() const { return m_osmDescription; }
  QVariantMap country() const { return m_country; }
  QString apiBackUrl() const { return m_apiBackUrl; }
  QString distance() const { return m_distance; }
  double azimuth() const { return m_azimuth; }
  QString bearing() const { return m_bearing; }
  QString shareText() const { return m_shareText; }
  QString geoUri() const { return m_geoUri; }
  QString coordinates() const { return m_coordinates; }
  QStringList coordinateValues() const { return m_coordinateValues; }
  QVariantList details() const { return m_details; }
  int openState() const { return m_openState; }
  QString openTitle() const { return m_openTitle; }
  QString openDescription() const { return m_openDescription; }
  QString openingHours() const { return m_openingHours; }
  QVariantList openingSchedule() const { return m_openingSchedule; }
  QString wikiDescription() const { return m_wikiDescription; }
  QString wikiUrl() const { return m_wikiUrl; }

  Q_INVOKABLE void close();
  // Cycles and persists the coordinates format.
  Q_INVOKABLE void nextCoordinatesFormat();

  void UpdateDistance();
  // Updates the "my position" subtitle; negative speed when unknown.
  void UpdateMyPosition(bool hasAltitude, double altitude, double speed);
  // Radians from true north.
  void SetNorth(double north);

signals:
  // Also for updates in place, e.g. of my position with every fix.
  void changed();
  // Another place was selected.
  void selected();
  void distanceChanged();
  void countryChanged();
  // A long tap on the empty map.
  void switchFullScreen();

private:
  void Update();
  void UpdateCoordinates(place_page::Info const & info);
  void UpdateOpeningHours(std::string_view openingHours);
  void UpdateCountry();

  Framework & m_framework;
  bool m_open = false;
  QString m_title;
  QString m_subtitle;
  QString m_address;
  QString m_secondaryTitle;
  QString m_osmDescription;
  QVariantMap m_country;
  QString m_apiBackUrl;
  std::string m_countryId;
  int m_storageSlot = 0;
  QString m_distance;
  double m_azimuth = -1.0;
  QString m_bearing;
  double m_north = 0.0;
  QString m_shareText;
  QString m_geoUri;
  QString m_coordinates;
  QStringList m_coordinateValues;
  QVariantList m_details;
  int m_openState = OpenUnknown;
  QString m_openTitle;
  QString m_openDescription;
  QString m_openingHours;
  QVariantList m_openingSchedule;
  QString m_wikiDescription;
  QString m_wikiUrl;
};
}  // namespace sailfish
