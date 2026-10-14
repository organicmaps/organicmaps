#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <string>
#include <string_view>
#include <vector>

class Framework;

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
  Q_PROPERTY(QString notes READ notes NOTIFY changed)
  // Empty unless a bookmark or track is selected.
  Q_PROPERTY(QString listName READ listName NOTIFY changed)
  Q_PROPERTY(quint64 listId READ listId NOTIFY changed)
  Q_PROPERTY(QString color READ color NOTIFY changed)
  Q_PROPERTY(bool isRoutePoint READ isRoutePoint NOTIFY changed)
  // A Routing::Road, 0 for none or one that can't be avoided.
  Q_PROPERTY(int roadToAvoid READ roadToAvoid NOTIFY changed)
  // See MissingMapInfo().
  Q_PROPERTY(QVariantMap country READ country NOTIFY countryChanged)
  Q_PROPERTY(QString apiBackUrl READ apiBackUrl NOTIFY changed)
  // {title, color, selected}, only when several tracks are under the tap.
  Q_PROPERTY(QVariantList trackCandidates READ trackCandidates NOTIFY changed)
  Q_PROPERTY(QString distance READ distance NOTIFY distanceChanged)
  // Degrees relative to the device heading, negative without a position.
  Q_PROPERTY(double azimuth READ azimuth NOTIFY distanceChanged)
  // From true north, like "123°".
  Q_PROPERTY(QString bearing READ bearing NOTIFY distanceChanged)
  // Empty for tracks.
  Q_PROPERTY(QString shareText READ shareText NOTIFY changed)
  Q_PROPERTY(QString geoUri READ geoUri NOTIFY changed)
  Q_PROPERTY(bool isBookmark READ isBookmark NOTIFY changed)
  // The bookmark was just deleted, and saving restores it.
  Q_PROPERTY(bool canRestoreBookmark READ canRestoreBookmark NOTIFY changed)
  Q_PROPERTY(bool isTrack READ isTrack NOTIFY changed)
  // A public transport route shown as a track: it belongs to no list and can't be edited, shared or deleted.
  Q_PROPERTY(bool isRelationTrack READ isRelationTrack NOTIFY changed)
  Q_PROPERTY(quint64 userMarkId READ userMarkId NOTIFY changed)
  Q_PROPERTY(bool canEdit READ canEdit NOTIFY changed)
  Q_PROPERTY(bool canAddPlace READ canAddPlace NOTIFY changed)
  Q_PROPERTY(bool canAddBusiness READ canAddBusiness NOTIFY changed)
  // Edit and add are disabled unless the map here is downloaded and editable.
  Q_PROPERTY(bool editable READ editable NOTIFY changed)
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
  // {label, value} rows.
  Q_PROPERTY(QVariantList trackStats READ trackStats NOTIFY changed)
  // ElevationChart() data; empty without altitudes.
  Q_PROPERTY(QVariantMap elevation READ elevation NOTIFY changed)
  // Meters along the track, negative when unset.
  Q_PROPERTY(double elevationActivePoint READ elevationActivePoint NOTIFY elevationPointsChanged)
  Q_PROPERTY(double elevationMyPosition READ elevationMyPosition NOTIFY elevationPointsChanged)
  // Transit routes of a stop: rich text refs with the shown one in bold, and {label, color} rows.
  Q_PROPERTY(QString routeRefs READ routeRefs NOTIFY changed)
  Q_PROPERTY(QVariantList routes READ routes NOTIFY changed)
  Q_PROPERTY(bool isTramStop READ isTramStop NOTIFY changed)
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
  QString notes() const { return m_notes; }
  QString listName() const { return m_listName; }
  quint64 listId() const { return m_listId; }
  QString color() const { return m_color; }
  bool isRoutePoint() const { return m_isRoutePoint; }
  int roadToAvoid() const { return m_roadToAvoid; }
  QVariantMap country() const { return m_country; }
  QString apiBackUrl() const { return m_apiBackUrl; }
  QVariantList trackCandidates() const { return m_trackCandidates; }
  QString distance() const { return m_distance; }
  double azimuth() const { return m_azimuth; }
  QString bearing() const { return m_bearing; }
  QString shareText() const { return m_shareText; }
  QString geoUri() const { return m_geoUri; }
  bool isBookmark() const { return m_isBookmark; }
  bool canRestoreBookmark() const;
  bool isTrack() const { return m_isTrack; }
  bool isRelationTrack() const { return m_isRelationTrack; }
  quint64 userMarkId() const { return m_userMarkId; }
  bool canEdit() const { return m_canEdit; }
  bool canAddPlace() const { return m_canAddPlace; }
  bool canAddBusiness() const { return m_canAddBusiness; }
  bool editable() const { return m_editable; }
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
  QString routeRefs() const { return m_routeRefs; }
  QVariantList routes() const { return m_routes; }
  bool isTramStop() const { return m_isTramStop; }
  QVariantList trackStats() const { return m_trackStats; }
  QVariantMap elevation() const { return m_elevation; }
  double elevationActivePoint() const;
  double elevationMyPosition() const;

  Q_INVOKABLE void close();
  // Cycles and persists the coordinates format.
  Q_INVOKABLE void nextCoordinatesFormat();
  // Saves the place to the last edited list, or deletes its bookmark.
  Q_INVOKABLE void toggleBookmark();
  Q_INVOKABLE void setElevationActivePoint(double distance);
  Q_INVOKABLE void showRoute(int index);
  // Moves the bookmark or track to another list.
  Q_INVOKABLE void setList(quint64 listId);
  // An index into the preset colors.
  Q_INVOKABLE void setColor(int colorIndex);
  Q_INVOKABLE void selectTrackCandidate(int index);

  void UpdateDistance();
  // Updates the "my position" subtitle; negative speed when unknown.
  void UpdateMyPosition(bool hasAltitude, double altitude, double speed);
  // Radians from true north.
  void SetNorth(double north);

signals:
  void changed();
  void distanceChanged();
  void elevationPointsChanged();
  void countryChanged();
  // A long tap on the empty map.
  void switchFullScreen();

private:
  void Update();
  void UpdateOpeningHours(std::string_view openingHours);
  void UpdateTrack();
  void UpdateRouteRefs();
  void UpdateList();
  void UpdateCountry();

  Framework & m_framework;
  bool m_open = false;
  QString m_title;
  QString m_subtitle;
  QString m_address;
  QString m_secondaryTitle;
  QString m_osmDescription;
  QString m_notes;
  QString m_listName;
  quint64 m_listId = 0;
  QString m_color;
  bool m_isRoutePoint = false;
  int m_roadToAvoid = 0;
  QVariantMap m_country;
  QString m_apiBackUrl;
  QVariantList m_trackCandidates;
  std::string m_countryId;
  int m_storageSlot = 0;
  QString m_distance;
  double m_azimuth = -1.0;
  QString m_bearing;
  double m_north = 0.0;
  QString m_shareText;
  QString m_geoUri;
  bool m_isBookmark = false;
  bool m_isTrack = false;
  bool m_isRelationTrack = false;
  quint64 m_userMarkId = 0;
  bool m_canEdit = false;
  bool m_canAddPlace = false;
  bool m_canAddBusiness = false;
  bool m_editable = false;
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
  QString m_routeRefs;
  QVariantList m_routes;
  std::vector<uint32_t> m_routeIds;
  bool m_isTramStop = false;
  QVariantList m_trackStats;
  QVariantMap m_elevation;
};
}  // namespace sailfish
