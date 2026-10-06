#include "sailfish/routing.hpp"

#include "sailfish/app_settings.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/elevation.hpp"
#include "sailfish/helpers.hpp"
#include "sailfish/voice_guide.hpp"

#include "map/elevation_info.hpp"
#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"
#include "map/transit/transit_display.hpp"

#include "routing/following_info.hpp"
#include "routing/lanes/lane_info.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_options.hpp"
#include "routing/turns.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "platform/distance.hpp"
#include "platform/get_text_by_id.hpp"
#include "platform/languages.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "storage/storage.hpp"

#include "geometry/mercator.hpp"

#include "base/sunrise_sunset.hpp"

#include <QColor>
#include <QDateTime>
#include <QGuiApplication>
#include <QVariantMap>

#include <algorithm>
#include <utility>
#include <vector>

#include <sailfishapp.h>

namespace sailfish
{
namespace
{
using routing::RouterResultCode;
using routing::RoutingOptions;

static_assert(Routing::Vehicle == static_cast<int>(routing::RouterType::Vehicle));
static_assert(Routing::Pedestrian == static_cast<int>(routing::RouterType::Pedestrian));
static_assert(Routing::Bicycle == static_cast<int>(routing::RouterType::Bicycle));
static_assert(Routing::Transit == static_cast<int>(routing::RouterType::Transit));
static_assert(Routing::Ruler == static_cast<int>(routing::RouterType::Ruler));
static_assert(Routing::Start == static_cast<int>(RouteMarkType::Start));
static_assert(Routing::Intermediate == static_cast<int>(RouteMarkType::Intermediate));
static_assert(Routing::Finish == static_cast<int>(RouteMarkType::Finish));
static_assert(Routing::Toll == static_cast<int>(RoutingOptions::Toll));
static_assert(Routing::Motorway == static_cast<int>(RoutingOptions::Motorway));
static_assert(Routing::Ferry == static_cast<int>(RoutingOptions::Ferry));
static_assert(Routing::Dirty == static_cast<int>(RoutingOptions::Dirty));

std::string_view constexpr kVoiceEnabledSetting = "SailfishVoiceInstructions";
// Empty for the app language.
std::string_view constexpr kVoiceLanguageSetting = "SailfishVoiceLanguage";
std::string_view constexpr kAnnounceStreetsSetting = "SailfishVoiceStreetNames";
// 0..1, shared with Android.
std::string_view constexpr kVoiceVolumeSetting = "TtsVolume";
// Shared with Android.
std::string_view constexpr kDisclaimerSetting = "IsDisclaimerApproved";

qint64 constexpr kDarkOutsideCheckIntervalMs = 60 * 1000;

std::string VoiceSetting(std::string const & language)
{
  return "SailfishSpeechNoteVoice_" + language;
}

QString ArrivalIn(int seconds)
{
  return FormatTime(QTime::currentTime().addSecs(seconds));
}

// Empty for a language without turn notifications.
QString VoiceLanguageName(std::string const & code)
{
  for (auto const & [lang, name] : routing::turns::sound::kLanguageList)
    if (lang == code)
      return ToQString(name);
  return {};
}

// Same icons as TransitStepType on Android.
QString TransitIcon(TransitType type)
{
  switch (type)
  {
  case TransitType::IntermediatePoint:
  case TransitType::Pedestrian: return QStringLiteral("ic_20px_route_planning_walk.webp");
  case TransitType::Subway: return QStringLiteral("ic_20px_route_planning_metro.webp");
  case TransitType::Train: return QStringLiteral("ic_20px_route_planning_train.webp");
  case TransitType::Monorail: return QStringLiteral("ic_20px_route_planning_monorail.webp");
  case TransitType::Tram:
  case TransitType::CableTram: return QStringLiteral("ic_20px_route_planning_tram.svg");
  case TransitType::LightRail:
  case TransitType::AerialLift:
  case TransitType::Funicular: return QStringLiteral("ic_20px_route_planning_lightrail.webp");
  default: return QStringLiteral("ic_20px_route_planning_bus.svg");
  }
}

// Same arrows as CarDirection and PedestrianTurnDirection on Android.
QString TurnIcon(routing::turns::CarDirection turn, uint32_t exitNum)
{
  using routing::turns::CarDirection;
  switch (turn)
  {
  case CarDirection::TurnRight: return QStringLiteral("ic_turn_right.webp");
  case CarDirection::TurnSharpRight: return QStringLiteral("ic_turn_right_sharp.webp");
  case CarDirection::TurnSlightRight: return QStringLiteral("ic_turn_right_slight.webp");
  case CarDirection::TurnLeft: return QStringLiteral("ic_turn_left.webp");
  case CarDirection::TurnSharpLeft: return QStringLiteral("ic_turn_left_sharp.webp");
  case CarDirection::TurnSlightLeft: return QStringLiteral("ic_turn_left_slight.webp");
  case CarDirection::UTurnLeft: return QStringLiteral("ic_turn_uleft.webp");
  case CarDirection::UTurnRight: return QStringLiteral("ic_turn_uright.webp");
  case CarDirection::EnterRoundAbout:
  case CarDirection::LeaveRoundAbout:
  case CarDirection::StayOnRoundAbout:
    return exitNum >= 1 && exitNum <= 12 ? QStringLiteral("ic_roundabout_exit_%1.svg").arg(exitNum)
                                         : QStringLiteral("ic_turn_round.svg");
  case CarDirection::ReachedYourDestination: return QStringLiteral("ic_turn_finish.webp");
  case CarDirection::ExitHighwayToLeft: return QStringLiteral("ic_exit_highway_to_left.webp");
  case CarDirection::ExitHighwayToRight: return QStringLiteral("ic_exit_highway_to_right.webp");
  default: return QStringLiteral("ic_turn_straight.webp");
  }
}

QString TurnIcon(routing::turns::PedestrianDirection turn)
{
  using routing::turns::PedestrianDirection;
  switch (turn)
  {
  case PedestrianDirection::TurnRight: return QStringLiteral("ic_turn_right.webp");
  case PedestrianDirection::TurnLeft: return QStringLiteral("ic_turn_left.webp");
  case PedestrianDirection::ReachedYourDestination: return QStringLiteral("ic_turn_finish.webp");
  default: return QStringLiteral("ic_turn_straight.webp");
  }
}

// Same arrows as LaneWay on Android.
QString LaneIcon(routing::turns::lanes::LaneWay way)
{
  using routing::turns::lanes::LaneWay;
  switch (way)
  {
  case LaneWay::ReverseLeft: return QStringLiteral("ic_turn_uleft.webp");
  case LaneWay::SharpLeft: return QStringLiteral("ic_turn_left_sharp.webp");
  case LaneWay::Left: return QStringLiteral("ic_turn_left.webp");
  case LaneWay::MergeToLeft:
  case LaneWay::SlightLeft: return QStringLiteral("ic_turn_left_slight.webp");
  case LaneWay::SlightRight:
  case LaneWay::MergeToRight: return QStringLiteral("ic_turn_right_slight.webp");
  case LaneWay::Right: return QStringLiteral("ic_turn_right.webp");
  case LaneWay::SharpRight: return QStringLiteral("ic_turn_right_sharp.webp");
  case LaneWay::ReverseRight: return QStringLiteral("ic_turn_uright.webp");
  default: return QStringLiteral("ic_turn_straight.webp");
  }
}

// Splits the street into {text} and {shield, text, color, textColor} parts. The core gives the shields position
// in code points.
QVariantList StreetParts(std::string const & street, routing::FollowingInfo::RoadShieldInfo const & info)
{
  auto const text = QString::fromStdString(street).toUcs4();
  auto const part = [&text](int from, int to)
  { return QString::fromUcs4(text.constData() + from, std::max(0, to - from)); };
  auto const & shields = info.m_targetRoadShields;
  int const start = std::min<int>(info.m_targetRoadShieldsPosition.first, text.size());
  int const end = std::min<int>(info.m_targetRoadShieldsPosition.second, text.size());
  QVariantList parts;
  if (shields.empty() || start >= end)
  {
    parts.append(QVariantMap{{"text", QString::fromStdString(street)}});
    return parts;
  }
  if (start > 0)
    parts.append(QVariantMap{{"text", part(0, start).trimmed()}});
  using ftypes::RoadShieldType;
  for (auto const & shield : shields)
  {
    // Colors from the map styles.
    QString color = QStringLiteral("#ffffff");
    QString textColor = QStringLiteral("#000000");
    switch (shield.m_type)
    {
    case RoadShieldType::Hidden: continue;
    case RoadShieldType::Generic_Green:
      color = "#309302";
      textColor = "#ffffff";
      break;
    case RoadShieldType::Generic_Blue:
      color = "#1a5ec1";
      textColor = "#ffffff";
      break;
    case RoadShieldType::Generic_Red:
      color = "#e63534";
      textColor = "#ffffff";
      break;
    case RoadShieldType::Generic_Orange: color = "#ffbe00"; break;
    case RoadShieldType::US_Interstate:
      color = "#1a5ec1";
      textColor = "#ffffff";
      break;
    case RoadShieldType::UK_Highway:
      color = "#309302";
      textColor = "#ffd400";
      break;
    default: break;
    }
    parts.append(QVariantMap{
        {"shield", true}, {"text", QString::fromStdString(shield.m_name)}, {"color", color}, {"textColor", textColor}});
  }
  if (end < text.size())
  {
    // Drops the " : " that joins the shields and the name.
    auto rest = part(end, text.size()).trimmed();
    if (rest.startsWith(':'))
      rest = rest.mid(1).trimmed();
    if (!rest.isEmpty())
      parts.append(QVariantMap{{"text", rest}});
  }
  return parts;
}

// An active lane shows its recommended way, others their first way.
QVariantList Lanes(routing::turns::lanes::LanesInfo const & lanes)
{
  using routing::turns::lanes::LaneWay;
  QVariantList result;
  for (auto const & lane : lanes)
  {
    bool const active = lane.recommendedWay != LaneWay::None;
    LaneWay way = lane.recommendedWay;
    for (uint8_t i = 0; !active && i < static_cast<uint8_t>(LaneWay::Count); ++i)
    {
      if (lane.laneWays.Contains(static_cast<LaneWay>(i)))
      {
        way = static_cast<LaneWay>(i);
        break;
      }
    }
    result.append(QVariantMap{{"icon", LaneIcon(way)}, {"active", active}});
  }
  return result;
}

// Distances between the points of a ruler route with stops, the stops in between, like
// RoutingBottomMenuController.pointsToRulerSteps() on Android.
QVariantList RulerSteps(std::vector<RouteMarkData> const & points)
{
  QVariantList steps;
  if (points.size() <= 2)
    return steps;
  for (size_t i = 1; i < points.size(); ++i)
  {
    if (i > 1)
    {
      auto const stop = QStringLiteral("route_point_%1.svg").arg(std::min<size_t>(i - 1, 9), 2, 10, QChar('0'));
      steps.append(QVariantMap{{"icon", stop}});
    }
    auto const distance =
        platform::Distance::CreateFormatted(mercator::DistanceOnEarth(points[i - 1].m_position, points[i].m_position));
    steps.append(QVariantMap{{"number", QString::fromStdString(distance.ToString())}});
  }
  return steps;
}
}  // namespace

Routing::Routing(Framework & framework, QObject * parent)
  : QObject(parent)
  , m_framework(framework)
  , m_voice(new VoiceGuide(this))
{
  m_beep.setMedia(SailfishApp::pathTo(QStringLiteral("sounds/speed_cams_beep.wav")));
  SetupVoice();
  m_framework.GetRoutingManager().SetRouteBuildingListener(
      [this](RouterResultCode code, storage::CountriesSet const & absent)
  {
    // BuildRoute() reports Cancelled synchronously, on this thread; a superseded build reports nothing.
    if (code == RouterResultCode::Cancelled)
    {
      m_buildCancelled = true;
      return;
    }
    QStringList ids;
    for (auto const & id : absent)
      ids.append(QString::fromStdString(id));
    // Queued: the result can arrive inside BuildRoute(), which OnRouteBuilt() may call again.
    QTimer::singleShot(0, this, [this, code = static_cast<int>(code), ids] { OnRouteBuilt(code, ids); });
  });

  // Rebuild once the missing maps are downloaded.
  m_storageSlot = m_framework.GetStorage().Subscribe([this](storage::CountryId const & id)
  {
    // A rebuild would close the route being followed.
    if (m_navigating || !m_missingMaps.contains(QString::fromStdString(id)))
      return;
    auto & storage = m_framework.GetStorage();
    for (auto const & missing : m_missingMaps)
      if (MapAttrs(storage, missing.toStdString()).m_status != storage::NodeStatus::OnDisk)
        return;
    Build();
  }, [](storage::CountryId const &, downloader::Progress const &) {});

  // A restored route starts at the position once it is found.
  m_framework.GetRoutingManager().SetRouteRecommendationListener([this](RoutingManager::Recommendation r)
  {
    if (r == RoutingManager::Recommendation::RebuildAfterPointsLoading && !m_navigating)
      setStartToMyPosition();
  });

  // Sunset and sunrise also matter outside navigation, for the scheduled appearance.
  m_darkOutsideTimer.setInterval(kDarkOutsideCheckIntervalMs);
  connect(&m_darkOutsideTimer, &QTimer::timeout, this, [this]
  {
    m_darkOutsideCheckMs = 0;
    UpdateDarkOutside();
  });
  m_darkOutsideTimer.start();
  UpdateDarkOutside();

  connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state)
  {
    if (state != Qt::ApplicationActive)
      SaveRouteForRestart();
  });
}

Routing::~Routing()
{
  SaveRouteForRestart();
  m_framework.GetStorage().Unsubscribe(m_storageSlot);
  // RoutingManager requires a listener.
  m_framework.GetRoutingManager().SetRouteBuildingListener([](RouterResultCode, storage::CountriesSet const &) {});
  m_framework.GetRoutingManager().SetRouteRecommendationListener(nullptr);
}

bool Routing::active() const
{
  return m_framework.GetRoutingManager().GetRoutePointsCount() > 0;
}

int Routing::routerType() const
{
  return static_cast<int>(m_framework.GetRoutingManager().GetLastUsedRouter());
}

void Routing::setRouterType(int type)
{
  if (type == routerType())
    return;
  UseRouter(type);
  Build();
}

void Routing::UseRouter(int type)
{
  auto & manager = m_framework.GetRoutingManager();
  auto const router = static_cast<routing::RouterType>(type);
  manager.SetRouter(router);
  manager.SetLastUsedRouter(router);
  emit routerTypeChanged();
}

QVariantList Routing::points() const
{
  QVariantList points;
  for (auto const & point : m_framework.GetRoutingManager().GetRoutePoints())
  {
    QVariantMap item;
    item["type"] = static_cast<int>(point.m_pointType);
    item["isMyPosition"] = point.m_isMyPosition;
    item["title"] = point.m_isMyPosition ? Localized("core_my_position") : QString::fromStdString(point.m_title);
    item["subtitle"] = QString::fromStdString(point.m_subTitle);
    points.append(item);
  }
  return points;
}

QString Routing::missingMapsSize() const
{
  qint64 size = 0;
  for (auto const & id : m_missingMaps)
    size += static_cast<qint64>(MapAttrs(m_framework.GetStorage(), id.toStdString()).m_mwmSize);
  return FormatSize(size);
}

int Routing::avoidRoads() const
{
  return RoutingOptions::LoadCarOptionsFromSettings().GetOptions();
}

void Routing::setAvoidRoads(int roads)
{
  RoutingOptions::SaveCarOptionsToSettings(RoutingOptions(static_cast<RoutingOptions::RoadType>(roads)));
  emit optionsChanged();
}

bool Routing::routeOptimization() const
{
  return RoutingOptions::LoadRouteOptimizationFromSettings();
}

void Routing::setRouteOptimization(bool enabled)
{
  RoutingOptions::SaveRouteOptimizationToSettings(enabled);
  emit optionsChanged();
}

bool Routing::canStart() const
{
  auto const type = routerType();
  return m_built && type != Ruler && type != Transit;
}

bool Routing::canReverse() const
{
  auto const points = m_framework.GetRoutingManager().GetRoutePoints();
  return m_built && points.front().m_pointType == RouteMarkType::Start &&
         points.back().m_pointType == RouteMarkType::Finish;
}

bool Routing::startIsMyPosition() const
{
  auto const points = m_framework.GetRoutingManager().GetRoutePoints();
  return !points.empty() && points.front().m_isMyPosition;
}

bool Routing::disclaimerAccepted() const
{
  return LoadSetting(kDisclaimerSetting, false);
}

void Routing::acceptDisclaimer()
{
  settings::Set(kDisclaimerSetting, true);
  emit disclaimerChanged();
}

bool Routing::canAddStop() const
{
  return m_framework.GetRoutingManager().GetRoutePointsCount() < RoutePointsLayout::kMaxRoutePointsCount;
}

void Routing::SaveRouteForRestart()
{
  auto & manager = m_framework.GetRoutingManager();
  if (m_navigating || (active() && m_built))
    manager.SaveRoutePoints();
  else if (active())
    // A restart must not bring back an earlier route of this planning.
    manager.DeleteSavedRoutePoints();
}

void Routing::restoreSavedRoute()
{
  auto & manager = m_framework.GetRoutingManager();
  if (active() || !manager.HasSavedRoutePoints())
    return;
  manager.LoadRoutePoints([this](bool success)
  {
    if (success)
      OnPointsChanged();
  });
}

void Routing::start()
{
  if (!canStart())
    return;
  auto & manager = m_framework.GetRoutingManager();
  // Navigation needs the route to start at the position; a finish at the position swaps with the start.
  auto points = manager.GetRoutePoints();
  if (!points.empty() && !points.front().m_isMyPosition)
  {
    m_startWhenBuilt = true;
    if (points.back().m_isMyPosition && points.back().m_pointType == RouteMarkType::Finish)
    {
      auto start = points.front();
      auto finish = points.back();
      start.m_pointType = RouteMarkType::Finish;
      finish.m_pointType = RouteMarkType::Start;
      manager.AddRoutePoint(std::move(finish), false /* optimize */);
      manager.AddRoutePoint(std::move(start), false /* optimize */);
      OnPointsChanged();
    }
    else
    {
      setStartToMyPosition();
    }
    return;
  }
  ClearElevationActivePoint();
  cancelPick();
  manager.FollowRoute();
  m_navigating = true;
  m_darkOutsideCheckMs = 0;
  // Picks up voices installed since the start.
  m_voice->Refresh();
  manager.EnableTurnNotifications(voiceEnabled());
  SetNavigationStyle(true);
  emit navigationChanged();
  UpdateNavigation(-1.0);
}

void Routing::stopNavigation()
{
  m_voice->Stop();
  EndNavigation();
}

void Routing::EndNavigation()
{
  m_navigating = false;
  m_navigation.clear();
  SetNavigationStyle(false);
  close();
  emit navigationChanged();
}

std::string Routing::AppVoiceLanguage() const
{
  auto const has = [](std::string const & code) { return !VoiceLanguageName(code).isEmpty(); };
  auto language = languages::GetCurrentTwine();
  if (!has(language))
    language = language.substr(0, language.find('-'));
  return has(language) ? language : "en";
}

void Routing::SetupVoice()
{
  // Voices are found in the background, Speech Note ones over D-Bus.
  connect(m_voice, &VoiceGuide::changed, this, [this]
  {
    auto & manager = m_framework.GetRoutingManager();
    if (m_voice->IsAvailable())
    {
      manager.SetTurnNotificationsLocale(m_voice->Language());
      m_voice->SetSpeechNoteVoice(
          QString::fromStdString(LoadSetting(VoiceSetting(m_voice->Language()), std::string())));
    }
    if (m_navigating)
      manager.EnableTurnNotifications(voiceEnabled());
    emit voiceChanged();
  });
  m_voice->SetPreferredLanguage(LoadSetting(kVoiceLanguageSetting, std::string()), AppVoiceLanguage());
  m_voice->SetVolume(voiceVolume());
  m_voice->Refresh();
}

bool Routing::voiceAvailable() const
{
  return m_voice->IsAvailable();
}

bool Routing::voiceEnabled() const
{
  return LoadSetting(kVoiceEnabledSetting, true) && m_voice->IsAvailable();
}

void Routing::setVoiceEnabled(bool enabled)
{
  settings::Set(kVoiceEnabledSetting, enabled);
  m_framework.GetRoutingManager().EnableTurnNotifications(voiceEnabled());
  if (!enabled)
    m_voice->Stop();
  emit voiceChanged();
}

QString Routing::voiceLanguage() const
{
  return QString::fromStdString(m_voice->Language());
}

void Routing::setVoiceLanguage(QString const & language)
{
  // The app language again is stored as no choice, so that it follows later app language changes.
  auto const code = language.toStdString();
  settings::Set(kVoiceLanguageSetting, code == AppVoiceLanguage() ? std::string() : code);
  m_voice->SetPreferredLanguage(code, AppVoiceLanguage());
}

QString Routing::voiceLanguageName() const
{
  return VoiceLanguageName(m_voice->Language());
}

int Routing::voiceLanguageIndex() const
{
  auto const languages = m_voice->Languages();
  auto const it = std::find(languages.begin(), languages.end(), m_voice->Language());
  return it == languages.end() ? -1 : static_cast<int>(it - languages.begin());
}

QVariantList Routing::voiceLanguages() const
{
  QVariantList languages;
  for (auto const & code : m_voice->Languages())
  {
    languages.append(QVariantMap{{"code", QString::fromStdString(code)},
                                 {"name", VoiceLanguageName(code)},
                                 {"speechNote", m_voice->HasSpeechNoteVoice(code)}});
  }
  return languages;
}

bool Routing::speechNoteInstalled() const
{
  return m_voice->IsSpeechNoteInstalled();
}

std::string Routing::WantedVoiceLanguage() const
{
  auto const preferred = LoadSetting(kVoiceLanguageSetting, std::string());
  return preferred.empty() ? AppVoiceLanguage() : preferred;
}

QString Routing::wantedVoiceLanguageName() const
{
  return VoiceLanguageName(WantedVoiceLanguage());
}

bool Routing::wantedHasSpeechNoteVoice() const
{
  return m_voice->HasSpeechNoteVoice(WantedVoiceLanguage());
}

bool Routing::announceStreets() const
{
  return LoadSetting(kAnnounceStreetsSetting, false);
}

void Routing::setAnnounceStreets(bool announce)
{
  settings::Set(kAnnounceStreetsSetting, announce);
  emit voiceChanged();
}

int Routing::voiceVolume() const
{
  return qRound(std::clamp(LoadSetting(kVoiceVolumeSetting, 1.0), 0.0, 1.0) * 100);
}

void Routing::setVoiceVolume(int volume)
{
  if (volume == voiceVolume())
    return;
  settings::Set(kVoiceVolumeSetting, volume / 100.0);
  m_voice->SetVolume(volume);
  emit voiceChanged();
}

QVariantList Routing::voices() const
{
  auto const voices = m_voice->SpeechNoteVoices();
  if (voices.size() < 2)
    return {};
  QVariantList result{QVariantMap{{"id", QString()}, {"name", Localized("auto")}}};
  for (auto const & [id, name] : voices)
    result.append(QVariantMap{{"id", id}, {"name", name}});
  return result;
}

QString Routing::voice() const
{
  return QString::fromStdString(LoadSetting(VoiceSetting(m_voice->Language()), std::string()));
}

void Routing::setVoice(QString const & voice)
{
  settings::Set(VoiceSetting(m_voice->Language()), voice.toStdString());
  m_voice->SetSpeechNoteVoice(voice);
  emit voiceChanged();
}

void Routing::refreshVoice()
{
  m_voice->Refresh();
}

void Routing::openSpeechNote()
{
  m_voice->OpenSpeechNote();
}

void Routing::testVoice()
{
  // Turn instructions rather than app texts, which are in the app language that may have no voice.
  std::pair<char const *, char const *> constexpr kTests[] = {{"in_200_meters", "make_a_left_turn"},
                                                              {"in_1_kilometer", "make_a_slight_right_turn"},
                                                              {nullptr, "you_have_reached_the_destination"}};
  if (!m_voice->IsAvailable())
    return;
  auto const text = platform::GetTextByIdFactory(platform::TextSource::TtsSound, m_voice->Language());
  if (!text)
    return;
  auto const & [distance, turn] = kTests[m_voiceTestIndex];
  QStringList speech;
  if (distance)
    speech.append(QString::fromStdString((*text)(distance)));
  speech.append(QString::fromStdString((*text)(turn)));
  m_voice->Speak(speech);
  m_voiceTestIndex = (m_voiceTestIndex + 1) % std::size(kTests);
}

void Routing::SetNavigationStyle(bool enabled)
{
  MapStyle const style = m_framework.GetMapStyle();
  bool const dark = MapStyleIsDark(style);
  if (enabled && routerType() == Vehicle)
    m_framework.SetMapStyle(dark ? MapStyleVehicleDark : MapStyleVehicleLight);
  else if (!enabled && (style == MapStyleVehicleDark || style == MapStyleVehicleLight))
    m_framework.SetMapStyle(BaseMapStyle(dark, Framework::LoadOutdoorsEnabled()));
}

void Routing::UpdateDarkOutside()
{
  auto const nowMs = QDateTime::currentMSecsSinceEpoch();
  if (m_darkOutsideCheckMs != 0 && nowMs - m_darkOutsideCheckMs < kDarkOutsideCheckIntervalMs)
    return;
  m_darkOutsideCheckMs = nowMs;
  bool dark = false;
  if (auto const position = m_framework.GetCurrentPosition())
  {
    auto const latLon = mercator::ToLatLon(*position);
    auto const dayTime = GetDayTime(static_cast<time_t>(nowMs / 1000), latLon.m_lat, latLon.m_lon);
    dark = dayTime == DayTimeType::Night || dayTime == DayTimeType::PolarNight;
  }
  else
  {
    auto const hour = QTime::currentTime().hour();
    dark = hour < 7 || hour >= 18;
  }
  if (dark != std::exchange(m_darkOutside, dark))
    emit darkOutsideChanged();
}

void Routing::UpdateNavigation(double speedMps)
{
  if (!m_navigating)
    return;
  UpdateDarkOutside();
  auto & manager = m_framework.GetRoutingManager();
  // Before the finish check, so that the arrival is announced.
  if (voiceEnabled())
  {
    std::vector<std::string> notifications;
    manager.GenerateNotifications(notifications, announceStreets());
    QStringList texts;
    for (auto const & text : notifications)
      texts.append(QString::fromStdString(text));
    m_voice->Speak(texts);
  }
  // The core takes the speed cameras setting into account.
  if (manager.GetSpeedCamManager().ShouldPlayBeepSignal())
    m_beep.play();
  if (manager.IsRouteFinished())
  {
    // Without stopping the voice, so that the arrival is heard.
    EndNavigation();
    return;
  }

  routing::FollowingInfo info;
  manager.GetRouteFollowingInfo(info);
  if (!info.IsValid())
    return;

  auto const units = measurement_utils::GetMeasurementUnits();
  bool const pedestrian = routerType() == Pedestrian;

  QVariantMap nav;
  nav["turnIcon"] = pedestrian ? TurnIcon(info.m_pedestrianTurn) : TurnIcon(info.m_turn, info.m_exitNum);
  nav["distanceToTurn"] = QString::fromStdString(info.m_distToTurn.ToString());
  nav["street"] = QString::fromStdString(info.m_nextStreetName);
  nav["streetParts"] = StreetParts(info.m_nextStreetName, info.m_nextStreetShields);
  nav["nextTurnIcon"] =
      !pedestrian && info.m_nextTurn != routing::turns::CarDirection::None ? TurnIcon(info.m_nextTurn, 0) : QString();
  nav["distanceLeftValue"] = QString::fromStdString(info.m_distToTarget.GetDistanceString());
  nav["distanceLeftUnits"] = QString::fromStdString(info.m_distToTarget.GetUnitsString());
  int const minutes = (info.m_time + 59) / 60;
  nav["hoursLeft"] = minutes / 60;
  nav["minutesLeft"] = minutes % 60;
  nav["hourUnits"] = Localized("hour");
  nav["minuteUnits"] = Localized("minute");
  nav["arrival"] = ArrivalIn(info.m_time);
  // Without a speed the last one stays shown, as on Android.
  nav["speed"] = speedMps >= 0 ? QString::fromStdString(measurement_utils::FormatSpeedNumeric(speedMps, units))
                               : m_navigation.value(QStringLiteral("speed"));
  nav["speedUnits"] = QString::fromStdString(platform::GetLocalizedSpeedUnits(units));
  nav["speedLimitExceeded"] = info.m_speedLimitMps > 0 && speedMps > info.m_speedLimitMps;
  nav["speedLimit"] = info.m_speedLimitMps > 0
                        ? QString::fromStdString(measurement_utils::FormatSpeedNumeric(info.m_speedLimitMps, units))
                        : QString();
  nav["speedCamLimitExceeded"] = manager.IsSpeedCamLimitExceeded();
  nav["lanes"] = Lanes(info.m_lanes);
  nav["progress"] = info.m_completionPercent / 100.0;
  m_navigation = nav;
  emit navigationChanged();
}

void Routing::routeFromPlace()
{
  AddPlacePoint(Start);
}

void Routing::routeToPlace()
{
  AddPlacePoint(Finish);
}

void Routing::addStopFromPlace()
{
  if (!canAddStop())
  {
    emit message(Localized("routing_max_stops_reached"));
    return;
  }
  AddPlacePoint(Intermediate);
}

void Routing::setStartToMyPosition()
{
  RouteMarkData start;
  start.m_pointType = RouteMarkType::Start;
  start.m_isMyPosition = true;
  if (auto const position = m_framework.GetCurrentPosition())
    start.m_position = *position;
  m_framework.GetRoutingManager().AddRoutePoint(std::move(start), false /* optimize */);
  OnPointsChanged();
}

void Routing::AddPlacePoint(int type)
{
  RouteMarkData point;
  if (!TakePlacePoint(point))
    return;
  point.m_pointType = static_cast<RouteMarkType>(type);
  AddPoint(std::move(point));
}

bool Routing::TakePlacePoint(RouteMarkData & point)
{
  if (!m_framework.HasPlacePageInfo())
    return false;
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  point.m_title = info.GetTitle();
  point.m_subTitle = info.GetSubtitle();
  point.m_isMyPosition = info.IsMyPosition();
  point.m_position = info.GetMercator();
  // The route panel takes the place of the place page.
  m_framework.DeactivateMapSelection();
  return true;
}

void Routing::AddPoint(RouteMarkData && point)
{
  auto & manager = m_framework.GetRoutingManager();
  auto const type = point.m_pointType;
  bool const optimize = type == RouteMarkType::Intermediate && RoutingOptions::LoadRouteOptimizationFromSettings();
  manager.AddRoutePoint(std::move(point), optimize);

  // A route to a place starts from the position unless a start was chosen.
  bool hasStart = false;
  for (auto const & p : manager.GetRoutePoints())
    hasStart |= p.m_pointType == RouteMarkType::Start;
  if (!hasStart && type == RouteMarkType::Finish)
    setStartToMyPosition();
  else
    OnPointsChanged();
}

void Routing::startPick(int type, int index)
{
  m_pickType = type;
  m_pickIndex = index;
  emit pickChanged();
}

void Routing::cancelPick()
{
  if (m_pickType < 0)
    return;
  m_pickType = -1;
  m_pickIndex = -1;
  emit pickChanged();
}

void Routing::PickPoint(RouteMarkData && point)
{
  if (m_pickType < 0)
    return;
  auto & manager = m_framework.GetRoutingManager();
  auto const points = manager.GetRoutePoints();
  auto const index = m_pickIndex;
  point.m_pointType = static_cast<RouteMarkType>(m_pickType);
  cancelPick();

  // A replaced stop keeps its place in the route.
  if (point.m_pointType == RouteMarkType::Intermediate && index >= 0 && index < static_cast<int>(points.size()))
  {
    auto const & old = points[static_cast<size_t>(index)];
    // In place; also handles "My position" elsewhere in the route, which it removes.
    manager.ReplaceRoutePoint(old.m_pointType, old.m_intermediateIndex, std::move(point));
    OnPointsChanged();
    return;
  }
  if (point.m_pointType == RouteMarkType::Intermediate && !canAddStop())
  {
    emit message(Localized("routing_max_stops_reached"));
    return;
  }
  AddPoint(std::move(point));
}

void Routing::pickPlace()
{
  RouteMarkData point;
  if (TakePlacePoint(point))
    PickPoint(std::move(point));
}

void Routing::pickPosition(double lat, double lon)
{
  RouteMarkData point;
  point.m_position = mercator::FromLatLon(lat, lon);
  point.m_title = m_framework.GetAddressAtPoint(point.m_position).FormatAddress();
  if (point.m_title.empty())
    point.m_title = FormatLatLon(lat, lon).toStdString();
  PickPoint(std::move(point));
}

void Routing::pickMyPosition()
{
  RouteMarkData point;
  point.m_isMyPosition = true;
  if (auto const position = m_framework.GetCurrentPosition())
    point.m_position = *position;
  PickPoint(std::move(point));
}

void Routing::planRoute(int routerType, QVariantList const & points)
{
  if (points.size() < 2)
    return;
  if (m_navigating)
    EndNavigation();
  auto & manager = m_framework.GetRoutingManager();
  manager.CloseRouting(true /* removeRoutePoints */);
  UseRouter(routerType);
  for (int i = 0; i < points.size(); ++i)
  {
    auto const point = points[i].toMap();
    RouteMarkData data;
    data.m_pointType = i == 0                 ? RouteMarkType::Start
                     : i + 1 == points.size() ? RouteMarkType::Finish
                                              : RouteMarkType::Intermediate;
    data.m_intermediateIndex = static_cast<size_t>(std::max(0, i - 1));
    data.m_title = point["name"].toString().toStdString();
    data.m_position = mercator::FromLatLon(point["lat"].toDouble(), point["lon"].toDouble());
    manager.AddRoutePoint(std::move(data), false /* optimize */);
  }
  m_framework.DeactivateMapSelection();
  OnPointsChanged();
}

void Routing::removePoint(int index)
{
  auto const points = m_framework.GetRoutingManager().GetRoutePoints();
  if (index < 0 || index >= static_cast<int>(points.size()))
    return;
  auto const & point = points[static_cast<size_t>(index)];
  m_framework.GetRoutingManager().RemoveRoutePoint(point.m_pointType, point.m_intermediateIndex);
  OnPointsChanged();
}

void Routing::removePlacePoint()
{
  if (!m_framework.HasPlacePageInfo() || !m_framework.GetCurrentPlacePageInfo().IsRoutePoint())
    return;
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  m_framework.GetRoutingManager().RemoveRoutePoint(info.GetRouteMarkType(), info.GetIntermediateIndex());
  m_framework.DeactivateMapSelection();
  OnPointsChanged();
}

void Routing::avoidRoad(int road)
{
  m_framework.DeactivateMapSelection();
  setAvoidRoads(avoidRoads() | road);
  // The options only change car routes.
  if (routerType() == Vehicle)
    Build();
}

void Routing::movePoint(int from, int to)
{
  auto & manager = m_framework.GetRoutingManager();
  auto const count = static_cast<int>(manager.GetRoutePointsCount());
  if (from < 0 || to < 0 || from >= count || to >= count || from == to)
    return;
  manager.MoveRoutePoint(static_cast<size_t>(from), static_cast<size_t>(to));
  OnPointsChanged();
}

void Routing::reverseRoute()
{
  if (m_framework.GetRoutingManager().ReverseRoutePoints())
    OnPointsChanged();
}

void Routing::applyOptions(int previousAvoidRoads, bool previousRouteOptimization)
{
  // A rebuild would end navigation, so changes then only apply to the next route.
  if (!active() || m_navigating)
    return;
  if (routeOptimization() && !previousRouteOptimization && m_framework.GetRoutingManager().OptimizeRoutePoints())
    OnPointsChanged();
  // The options only change car routes.
  else if (avoidRoads() != previousAvoidRoads && routerType() == Vehicle)
    Build();
}

void Routing::downloadMissingMaps()
{
  for (auto const & id : m_missingMaps)
    m_framework.GetStorage().DownloadNode(id.toStdString());
}

void Routing::saveRoute()
{
  if (!m_built || m_routeSaved)
    return;
  // The chart mark would stay on the saved track.
  ClearElevationActivePoint();
  m_framework.SaveRoute();
  m_routeSaved = true;
  emit stateChanged();
}

void Routing::setElevationActivePoint(double distance)
{
  auto const point = m_framework.GetRoutingManager().GetRoutePointAtDistance(distance);
  auto engine = m_framework.GetDrapeEngine();
  if (!point || !engine)
    return;
  engine->SelectObject(df::SelectionShape::ESelectedObject::OBJECT_TRACK, *point, FeatureID(), false /* isAnim */,
                       false /* isGeometrySelectionAllowed */, true /* isSelectionShapeVisible */);
  m_elevationActivePoint = distance;
  emit elevationActivePointChanged();
}

void Routing::ClearElevationActivePoint()
{
  if (m_elevationActivePoint < 0)
    return;
  // A place tapped since owns the selection now.
  if (auto engine = m_framework.GetDrapeEngine(); engine && !m_framework.HasPlacePageInfo())
    engine->DeselectObject(false /* restoreViewport */);
  m_elevationActivePoint = -1;
  emit elevationActivePointChanged();
}

void Routing::LoadElevation()
{
  // Car routes have no altitudes; the ruler has no roads.
  auto const type = routerType();
  ElevationInfo elevation;
  if (type == Vehicle || type == Ruler || !m_framework.GetRoutingManager().GetRouteElevationInfo(elevation) ||
      elevation.GetSize() < 2)
    return;

  auto const info = elevation.CalculateAltitudesInfo(ElevationInfo::kDefThresholdMWM);
  m_elevation = ElevationChart(elevation, info.m_minAltitude, info.m_maxAltitude);
  m_ascentDescent = QStringLiteral("↗\u00A0%1 ↘\u00A0%2")
                        .arg(QString::fromStdString(platform::Distance::FormatAltitude(info.GetTotalAscent())),
                             QString::fromStdString(platform::Distance::FormatAltitude(info.GetTotalDescent())));
}

void Routing::close()
{
  cancelPick();
  m_startWhenBuilt = false;
  m_framework.GetRoutingManager().CloseRouting(true /* removeRoutePoints */);
  m_framework.GetRoutingManager().DeleteSavedRoutePoints();
  m_building = false;
  ClearResult();
  emit pointsChanged();
  emit stateChanged();
}

void Routing::ClearResult()
{
  m_built = false;
  m_routeSaved = false;
  m_optionsError = false;
  m_summary.clear();
  m_arrival.clear();
  m_walkingDistance.clear();
  m_transitSteps.clear();
  m_missingMaps.clear();
  m_elevation.clear();
  m_ascentDescent.clear();
  ClearElevationActivePoint();
  SetError({}, {});
}

void Routing::OnPointsChanged()
{
  emit pointsChanged();
  if (!active())
  {
    close();
    return;
  }
  Build();
}

void Routing::Build()
{
  auto & manager = m_framework.GetRoutingManager();
  ClearResult();
  if (manager.GetRoutePointsCount() < 2)
  {
    m_building = false;
    manager.RemoveRoute(false /* deactivateFollowing */);
    emit stateChanged();
    return;
  }
  manager.SetRouter(manager.GetLastUsedRouter());
  m_building = true;
  m_buildCancelled = false;
  emit stateChanged();
  manager.BuildRoute();
  // E.g. two points at the same place: the core closes the route without building it.
  if (m_buildCancelled)
  {
    m_building = false;
    emit stateChanged();
  }
}

void Routing::SetError(QString const & title, QString const & message)
{
  m_errorTitle = title;
  m_errorMessage = message;
}

void Routing::OnRouteBuilt(int code, QStringList const & absentCountries)
{
  auto const result = static_cast<RouterResultCode>(code);
  m_building = false;
  switch (result)
  {
  case RouterResultCode::NoError:
  case RouterResultCode::HasWarnings:
  {
    m_built = true;
    routing::FollowingInfo info;
    m_framework.GetRoutingManager().GetRouteFollowingInfo(info);
    QString const distance = QString::fromStdString(info.m_distToTarget.ToString());
    if (routerType() == Transit)
    {
      auto const transit = m_framework.GetRoutingManager().GetTransitRouteInfo();
      m_summary = FormatDuration(transit.m_totalTimeInSec);
      m_arrival = ArrivalIn(transit.m_totalTimeInSec);
      if (transit.m_totalPedestrianTimeInSec > 0)
      {
        m_walkingDistance =
            QString::fromStdString(transit.m_totalPedestrianDistanceStr + " " + transit.m_totalPedestrianUnitsSuffix);
      }
      for (auto const & step : transit.m_steps)
      {
        bool const walk = step.m_type == TransitType::Pedestrian || step.m_type == TransitType::IntermediatePoint;
        QVariantMap item;
        item["icon"] = TransitIcon(step.m_type);
        item["number"] = QString::fromStdString(step.m_number);
        if (!walk)
        {
          auto const color = QColor::fromRgba(step.m_colorARGB);
          item["color"] = color.name();
          // Readable on light line colors too.
          item["textColor"] = qGray(color.rgb()) > 150 ? QStringLiteral("#000000") : QStringLiteral("#ffffff");
        }
        m_transitSteps.append(item);
      }
    }
    else
    {
      m_summary = routerType() == Ruler ? Localized("placepage_distance") + ": " + distance
                                        : JoinDetails({FormatDuration(info.m_time), distance});
      if (routerType() == Ruler)
        m_transitSteps = RulerSteps(m_framework.GetRoutingManager().GetRoutePoints());
      else
        m_arrival = ArrivalIn(info.m_time);
      LoadElevation();
    }
    break;
  }
  // Same messages as ResultCodesHelper on Android.
  case RouterResultCode::NoCurrentPosition:
    SetError(Localized("dialog_routing_location_turn_on"), Localized("dialog_routing_location_unknown_turn_on"));
    break;
  case RouterResultCode::StartPointNotFound:
    SetError(Localized("dialog_routing_change_start"), Localized("dialog_routing_start_not_determined"));
    break;
  case RouterResultCode::EndPointNotFound:
    SetError(Localized("dialog_routing_change_end"), Localized("dialog_routing_end_not_determined"));
    break;
  case RouterResultCode::IntermediatePointNotFound:
    SetError(Localized("dialog_routing_change_intermediate"), Localized("dialog_routing_intermediate_not_determined"));
    break;
  case RouterResultCode::PointsInDifferentMWM: SetError({}, Localized("routing_failed_cross_mwm_building")); break;
  case RouterResultCode::FileTooOld:
    SetError(Localized("downloader_update_maps"), Localized("downloader_mwm_migration_dialog"));
    break;
  case RouterResultCode::TransitRouteNotFoundNoNetwork: SetError({}, Localized("transit_not_found")); break;
  case RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
    SetError(Localized("dialog_pedestrian_route_is_long_header"), Localized("dialog_pedestrian_route_is_long_message"));
    break;
  case RouterResultCode::NeedMoreMaps:
    m_missingMaps = absentCountries;
    SetError(Localized("dialog_routing_download_and_build_cross_route"),
             Localized("dialog_routing_download_cross_route"));
    break;
  case RouterResultCode::RouteNotFound:
  case RouterResultCode::RouteNotFoundRedressRouteError:
    m_missingMaps = absentCountries;
    if (m_missingMaps.isEmpty())
      SetError(Localized("dialog_routing_unable_locate_route"), Localized("dialog_routing_cant_build_route"));
    else
      SetError(Localized("routing_download_maps_along"), Localized("routing_requires_all_map"));
    break;
  default: SetError(Localized("dialog_routing_system_error"), Localized("dialog_routing_application_error")); break;
  }
  // Avoided roads explain a failed car route best.
  if (!m_built && result != RouterResultCode::NeedMoreMaps && m_missingMaps.isEmpty() && routerType() == Vehicle &&
      avoidRoads() != 0)
  {
    m_optionsError = true;
    SetError(Localized("unable_to_calc_alert_title"), Localized("unable_to_calc_alert_subtitle"));
  }
  emit stateChanged();
  if (std::exchange(m_startWhenBuilt, false) && m_built)
    start();
}
}  // namespace sailfish
