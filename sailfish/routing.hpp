#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <string>

class Framework;
struct RouteMarkData;

namespace sailfish
{
class VoiceGuide;

// Route planning and navigation. Kept free of map headers, which Qt 5.6 moc can't parse.
class Routing : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool active READ active NOTIFY pointsChanged)
  Q_PROPERTY(int routerType READ routerType WRITE setRouterType NOTIFY routerTypeChanged)
  // {type, title, subtitle, isMyPosition}; type is a PointType.
  Q_PROPERTY(QVariantList points READ points NOTIFY pointsChanged)
  Q_PROPERTY(bool building READ building NOTIFY stateChanged)
  Q_PROPERTY(bool built READ built NOTIFY stateChanged)
  Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)
  Q_PROPERTY(QString arrival READ arrival NOTIFY stateChanged)
  Q_PROPERTY(QString walkingDistance READ walkingDistance NOTIFY stateChanged)
  // Transit legs as {icon, number, color, textColor}, or the distances between the points and stops of a ruler
  // route.
  Q_PROPERTY(QVariantList transitSteps READ transitSteps NOTIFY stateChanged)
  Q_PROPERTY(QString errorTitle READ errorTitle NOTIFY stateChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
  Q_PROPERTY(QStringList missingMaps READ missingMaps NOTIFY stateChanged)
  Q_PROPERTY(QString missingMapsSize READ missingMapsSize NOTIFY stateChanged)
  Q_PROPERTY(bool downloadingMissingMaps READ downloadingMissingMaps NOTIFY missingMapsProgressChanged)
  Q_PROPERTY(double missingMapsProgress READ missingMapsProgress NOTIFY missingMapsProgressChanged)
  Q_PROPERTY(QVariantMap elevation READ elevation NOTIFY stateChanged)
  Q_PROPERTY(QString ascentDescent READ ascentDescent NOTIFY stateChanged)
  // Distance of the point marked on the route, -1 without one.
  Q_PROPERTY(double elevationActivePoint READ elevationActivePoint NOTIFY elevationActivePointChanged)
  // Mask of avoided Road values.
  Q_PROPERTY(int avoidRoads READ avoidRoads WRITE setAvoidRoads NOTIFY optionsChanged)
  Q_PROPERTY(bool routeOptimization READ routeOptimization WRITE setRouteOptimization NOTIFY optionsChanged)
  Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
  Q_PROPERTY(bool canReverse READ canReverse NOTIFY stateChanged)
  Q_PROPERTY(bool startIsMyPosition READ startIsMyPosition NOTIFY pointsChanged)
  Q_PROPERTY(bool disclaimerAccepted READ disclaimerAccepted NOTIFY disclaimerChanged)
  Q_PROPERTY(bool canAddStop READ canAddStop NOTIFY pointsChanged)
  // The PointType of the slot waiting for a place, -1 for none, and the index of the point to replace, -1 to add.
  Q_PROPERTY(int pickType READ pickType NOTIFY pickChanged)
  Q_PROPERTY(int pickIndex READ pickIndex NOTIFY pickChanged)
  // The route failed while roads are avoided, the likely cause.
  Q_PROPERTY(bool optionsError READ optionsError NOTIFY stateChanged)
  Q_PROPERTY(bool routeSaved READ routeSaved NOTIFY stateChanged)
  Q_PROPERTY(bool navigating READ navigating NOTIFY navigationChanged)
  // turnIcon, distanceToTurn, street, streetParts, nextTurnIcon, hoursLeft, minutesLeft, hourUnits, minuteUnits,
  // arrival, distanceLeftValue, distanceLeftUnits, speed, speedUnits, speedLimit, speedLimitExceeded,
  // speedCamLimitExceeded, lanes as {icon, active} and progress (0..1).
  Q_PROPERTY(QVariantMap navigation READ navigation NOTIFY navigationChanged)
  // Between sunset and sunrise at the position, or by the clock (7 to 18 is day) without one.
  Q_PROPERTY(bool darkOutside READ darkOutside NOTIFY darkOutsideChanged)
  Q_PROPERTY(bool voiceAvailable READ voiceAvailable NOTIFY voiceChanged)
  Q_PROPERTY(bool voiceEnabled READ voiceEnabled WRITE setVoiceEnabled NOTIFY voiceChanged)
  // A code of voiceLanguages; setting it overrides the app language.
  Q_PROPERTY(QString voiceLanguage READ voiceLanguage WRITE setVoiceLanguage NOTIFY voiceChanged)
  Q_PROPERTY(QString voiceLanguageName READ voiceLanguageName NOTIFY voiceChanged)
  // -1 without a voice.
  Q_PROPERTY(int voiceLanguageIndex READ voiceLanguageIndex NOTIFY voiceChanged)
  // {code, name, speechNote}.
  Q_PROPERTY(QVariantList voiceLanguages READ voiceLanguages NOTIFY voiceChanged)
  Q_PROPERTY(bool speechNoteInstalled READ speechNoteInstalled NOTIFY voiceChanged)
  // The chosen or the app language, even without a voice.
  Q_PROPERTY(QString wantedVoiceLanguageName READ wantedVoiceLanguageName NOTIFY voiceChanged)
  Q_PROPERTY(bool wantedHasSpeechNoteVoice READ wantedHasSpeechNoteVoice NOTIFY voiceChanged)
  Q_PROPERTY(bool announceStreets READ announceStreets WRITE setAnnounceStreets NOTIFY voiceChanged)
  // 0..100.
  Q_PROPERTY(int voiceVolume READ voiceVolume WRITE setVoiceVolume NOTIFY voiceChanged)
  // Speech Note voices as {id, name}, the first being the default with an empty id; empty without a choice. The
  // chosen voice is kept per language.
  Q_PROPERTY(QVariantList voices READ voices NOTIFY voiceChanged)
  Q_PROPERTY(QString voice READ voice WRITE setVoice NOTIFY voiceChanged)

public:
  // Mirrors routing::RouterType.
  enum RouterType
  {
    Vehicle,
    Pedestrian,
    Bicycle,
    Transit,
    Ruler
  };
  Q_ENUM(RouterType)

  // Mirrors RouteMarkType.
  enum PointType
  {
    Start,
    Intermediate,
    Finish
  };
  Q_ENUM(PointType)

  // RoutingOptions::Road values.
  enum Road
  {
    Toll = 1 << 1,
    Motorway = 1 << 2,
    Ferry = 1 << 3,
    Dirty = 1 << 4
  };
  Q_ENUM(Road)

  explicit Routing(Framework & framework, QObject * parent = nullptr);
  ~Routing() override;

  bool active() const;
  int routerType() const;
  void setRouterType(int type);
  QVariantList points() const;
  bool building() const { return m_building; }
  bool built() const { return m_built; }
  QString summary() const { return m_summary; }
  QString arrival() const { return m_arrival; }
  QString walkingDistance() const { return m_walkingDistance; }
  QVariantList transitSteps() const { return m_transitSteps; }
  QString errorTitle() const { return m_errorTitle; }
  QString errorMessage() const { return m_errorMessage; }
  QStringList missingMaps() const { return m_missingMaps; }
  QString missingMapsSize() const;
  bool downloadingMissingMaps() const;
  // Of all the missing maps together, from 0 to 1.
  double missingMapsProgress() const;
  QVariantMap elevation() const { return m_elevation; }
  QString ascentDescent() const { return m_ascentDescent; }
  double elevationActivePoint() const { return m_elevationActivePoint; }
  int avoidRoads() const;
  void setAvoidRoads(int roads);
  bool routeOptimization() const;
  void setRouteOptimization(bool enabled);
  bool canStart() const;
  bool canReverse() const;
  bool startIsMyPosition() const;
  bool disclaimerAccepted() const;
  bool canAddStop() const;
  bool optionsError() const { return m_optionsError; }
  int pickType() const { return m_pickType; }
  int pickIndex() const { return m_pickIndex; }
  bool routeSaved() const { return m_routeSaved; }
  bool navigating() const { return m_navigating; }
  QVariantMap navigation() const { return m_navigation; }
  bool darkOutside() const { return m_darkOutside; }

  // Called on every location update.
  void UpdateNavigation(double speedMps);

  // Use the place shown in the place page.
  Q_INVOKABLE void routeFromPlace();
  Q_INVOKABLE void routeToPlace();
  Q_INVOKABLE void addStopFromPlace();
  Q_INVOKABLE void setStartToMyPosition();
  // Replaces the route with points as {lat, lon, name}, from start to finish.
  Q_INVOKABLE void planRoute(int routerType, QVariantList const & points);
  Q_INVOKABLE void removePoint(int index);
  // Act on the route point or road warning shown in the place page.
  Q_INVOKABLE void removePlacePoint();
  Q_INVOKABLE void avoidRoad(int road);
  Q_INVOKABLE void movePoint(int from, int to);
  // Reverses the route with its stops and rebuilds it.
  Q_INVOKABLE void reverseRoute();
  // Leaving the routing options, given as they were on entering: rebuilds the planned route once if they changed, and
  // reorders the stops if route optimization was turned on.
  Q_INVOKABLE void applyOptions(int previousAvoidRoads, bool previousRouteOptimization);
  Q_INVOKABLE void downloadMissingMaps();
  Q_INVOKABLE void cancelMissingMaps();
  Q_INVOKABLE void acceptDisclaimer();
  Q_INVOKABLE void startPick(int type, int index);
  Q_INVOKABLE void cancelPick();
  Q_INVOKABLE void pickPlace();
  Q_INVOKABLE void pickPosition(double lat, double lon);
  Q_INVOKABLE void pickMyPosition();
  // Brings back the route saved by SaveRouteForRestart().
  Q_INVOKABLE void restoreSavedRoute();
  // Saves the built route as a track.
  Q_INVOKABLE void saveRoute();
  Q_INVOKABLE void setElevationActivePoint(double distance);
  Q_INVOKABLE void close();
  // Moves the start to the position first if needed.
  Q_INVOKABLE void start();
  // Also closes the route.
  Q_INVOKABLE void stopNavigation();

  bool voiceAvailable() const;
  bool voiceEnabled() const;
  void setVoiceEnabled(bool enabled);
  QString voiceLanguage() const;
  void setVoiceLanguage(QString const & language);
  QString voiceLanguageName() const;
  int voiceLanguageIndex() const;
  QVariantList voiceLanguages() const;
  bool speechNoteInstalled() const;
  QString wantedVoiceLanguageName() const;
  bool wantedHasSpeechNoteVoice() const;
  bool announceStreets() const;
  void setAnnounceStreets(bool announce);
  int voiceVolume() const;
  QVariantList voices() const;
  QString voice() const;
  void setVoice(QString const & voice);
  void setVoiceVolume(int volume);
  // E.g. after installing Speech Note or a voice.
  Q_INVOKABLE void refreshVoice();
  Q_INVOKABLE void openSpeechNote();
  Q_INVOKABLE void testVoice();

signals:
  void pointsChanged();
  void routerTypeChanged();
  void stateChanged();
  void optionsChanged();
  void navigationChanged();
  void voiceChanged();
  void disclaimerChanged();
  void pickChanged();
  void message(QString const & text);
  void elevationActivePointChanged();
  void missingMapsProgressChanged();
  void darkOutsideChanged();

private:
  void AddPlacePoint(int type);
  // Also closes the place page; false without a place.
  bool TakePlacePoint(RouteMarkData & point);
  // Also makes it the default of new routes.
  void UseRouter(int type);
  // A finish without a start starts from the position.
  void AddPoint(RouteMarkData && point);
  void PickPoint(RouteMarkData && point);
  void OnPointsChanged();
  void Build();
  void ClearResult();
  void OnRouteBuilt(int code, QStringList const & absentCountries);
  void SetError(QString const & title, QString const & message);
  void SaveRouteForRestart();
  void LoadElevation();
  void ClearElevationActivePoint();
  void SetNavigationStyle(bool enabled);
  void UpdateDarkOutside();
  void EndNavigation();
  void SetupVoice();
  std::string AppVoiceLanguage() const;
  std::string WantedVoiceLanguage() const;

  Framework & m_framework;
  bool m_building = false;
  bool m_buildCancelled = false;
  bool m_built = false;
  bool m_routeSaved = false;
  bool m_optionsError = false;
  int m_pickType = -1;
  int m_pickIndex = -1;
  QString m_summary;
  QString m_arrival;
  QString m_walkingDistance;
  QVariantList m_transitSteps;
  QString m_errorTitle;
  QString m_errorMessage;
  QStringList m_missingMaps;
  QVariantMap m_elevation;
  QString m_ascentDescent;
  double m_elevationActivePoint = -1;
  int m_storageSlot = 0;
  bool m_navigating = false;
  // START moved the start to the position; navigate once the route is rebuilt.
  bool m_startWhenBuilt = false;
  QVariantMap m_navigation;
  bool m_darkOutside = false;
  // Sunset needs no check on every location update.
  qint64 m_darkOutsideCheckMs = 0;
  QTimer m_darkOutsideTimer;
  VoiceGuide * m_voice = nullptr;
  // Speed camera beep.
  QMediaPlayer m_beep;
  int m_voiceTestIndex = 0;
};
}  // namespace sailfish
