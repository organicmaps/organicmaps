#include "sailfish/place_page.hpp"

#include "sailfish/countries_model.hpp"
#include "sailfish/elevation.hpp"
#include "sailfish/helpers.hpp"
#include "sailfish/opening_hours_editor.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/elevation_info.hpp"
#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_mark.hpp"
#include "map/track.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "ge0/url_generator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

#include "opening_hours/opening_hours.hpp"

#include "routing/routing_options.hpp"

#include "storage/storage.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/settings.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <QColor>
#include <QDateTime>
#include <QLocale>
#include <QRegularExpression>
#include <QUrl>
#include <QVariantMap>

#include <cmath>
#include <ctime>

namespace sailfish
{
namespace
{
using feature::Metadata;

// Same setting and default as the desktop place page.
std::string_view constexpr kCoordinatesFormatSetting = "CoordinatesFormat";

// The week from today, grouping days with the same hours.
QVariantList MakeWeekSchedule(editor::ui::TimeTableSet & tts)
{
  std::vector<editor::ui::TimeTable> tables;
  for (size_t i = 0; i < tts.Size(); ++i)
    tables.push_back(tts.Get(i));
  // osmoh::Weekday counts from Sunday = 1, Qt from Monday = 1.
  int const today = QDate::currentDate().dayOfWeek() % 7 + 1;
  std::vector<int> week;
  for (int i = 0; i < 7; ++i)
    week.push_back((today - 1 + i) % 7 + 1);
  auto const find = [&tables](int day) -> editor::ui::TimeTable const *
  {
    for (auto const & tt : tables)
      if (tt.GetOpeningDays().count(static_cast<osmoh::Weekday>(day)))
        return &tt;
    return nullptr;
  };

  QVariantList schedule;
  for (size_t i = 0; i < week.size(); ++i)
  {
    size_t const first = i;
    auto const * tt = find(week[i]);
    if (tt)
    {
      while (i + 1 < week.size() && tt->GetOpeningDays().count(static_cast<osmoh::Weekday>(week[i + 1])))
        ++i;
    }
    else
    {
      // Closed days run until the next open one.
      while (i + 1 < week.size() && !find(week[i + 1]))
        ++i;
    }
    QString days = ShortDayName(week[first]);
    if (i != first)
      days += "-" + ShortDayName(week[i]);
    QString hours = !tt                     ? Localized("day_off")
                  : tt->IsTwentyFourHours() ? Localized("editor_time_allday")
                                            : FormatShifts(*tt, "\n", true /* namedNoonMidnight */);
    if (hours.isEmpty())
      hours = Localized("day_off");
    schedule.append(QVariantMap{{"days", days}, {"hours", hours}, {"today", first == 0}});
  }
  return schedule;
}

int32_t SavedCoordinatesFormat()
{
  return LoadSetting(kCoordinatesFormatSetting, static_cast<int32_t>(place_page::CoordinatesFormat::LatLonDecimal));
}
}  // namespace

PlacePage::PlacePage(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  m_framework.SetPlacePageListeners([this] { Update(); }, [this]
  {
    m_open = false;
    emit changed();
  }, [this] { Update(); }, [this] { emit switchFullScreen(); });
  auto & manager = m_framework.GetBookmarkManager();
  manager.SetElevationActivePointChangedCallback([this](kml::TrackId, double) { emit elevationPointsChanged(); });
  manager.SetElevationMyPositionChangedCallback([this](kml::TrackId, double) { emit elevationPointsChanged(); });
  m_storageSlot = m_framework.GetStorage().Subscribe(
      [this](storage::CountryId const & id)
  {
    if (id == m_countryId)
      UpdateCountry();
  }, [this](storage::CountryId const & id, downloader::Progress const &)
  {
    if (id == m_countryId)
      UpdateCountry();
  });
}

PlacePage::~PlacePage()
{
  m_framework.GetStorage().Unsubscribe(m_storageSlot);
  m_framework.SetPlacePageListeners({}, {}, {}, {});
  auto & manager = m_framework.GetBookmarkManager();
  manager.SetElevationActivePointChangedCallback({});
  manager.SetElevationMyPositionChangedCallback({});
}

double PlacePage::elevationActivePoint() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return m_isTrack && manager.HasTrack(m_userMarkId) ? manager.GetElevationActivePoint(m_userMarkId) : -1.0;
}

double PlacePage::elevationMyPosition() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return m_isTrack && manager.HasTrack(m_userMarkId) ? manager.GetElevationMyPosition(m_userMarkId) : -1.0;
}

void PlacePage::setElevationActivePoint(double distance)
{
  if (m_isTrack && m_framework.GetBookmarkManager().HasTrack(m_userMarkId))
    m_framework.GetBookmarkManager().SetElevationActivePoint(m_userMarkId, distance);
}

void PlacePage::close()
{
  m_framework.DeactivateMapSelection();
}

void PlacePage::nextCoordinatesFormat()
{
  if (!m_framework.HasPlacePageInfo())
    return;
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  auto const entries = place_page::GetAvailableCoordinateFormats(info.GetLatLon(), info.GetCountryId());
  settings::Set(kCoordinatesFormatSetting,
                static_cast<int32_t>(place_page::NextCoordinateFormat(entries, SavedCoordinatesFormat())));
  Update();
}

void PlacePage::toggleBookmark()
{
  if (!m_framework.HasPlacePageInfo())
    return;

  auto const & info = m_framework.GetCurrentPlacePageInfo();
  auto & manager = m_framework.GetBookmarkManager();
  auto buildInfo = info.GetBuildInfo();
  if (info.IsBookmark())
  {
    // Deleted from the bookmark list while the page was open.
    if (!IsSavedUserItem(manager, info.GetBookmarkId(), false /* isTrack */))
    {
      m_framework.DeactivateMapSelection();
      return;
    }
    manager.GetEditSession().DeleteBookmark(info.GetBookmarkId());
    buildInfo.m_match = place_page::BuildInfo::Match::FeatureOnly;
    buildInfo.m_userMarkId = kml::kInvalidMarkId;
    buildInfo.m_source = place_page::BuildInfo::Source::Other;
  }
  else
  {
    kml::BookmarkData data;
    data.m_name = info.FormatNewBookmarkName();
    data.m_point = info.GetMercator();
    data.m_color = m_framework.LastEditedBMColor();
    if (info.IsFeature())
      SaveFeatureTypes(info.GetTypes(), data);
    auto const * bookmark =
        manager.GetEditSession().CreateBookmark(std::move(data), m_framework.LastEditedBMCategory());
    buildInfo.m_match = place_page::BuildInfo::Match::Everything;
    buildInfo.m_userMarkId = bookmark->GetId();
  }
  // Reselecting the place updates the page.
  m_framework.UpdatePlacePageInfoForCurrentSelection(buildInfo);
}

bool PlacePage::canRestoreBookmark() const
{
  return m_framework.GetBookmarkManager().HasRecentlyDeletedBookmark();
}

void PlacePage::Update()
{
  if (!m_framework.HasPlacePageInfo())
    return;

  auto const & info = m_framework.GetCurrentPlacePageInfo();
  m_open = true;
  m_title = QString::fromStdString(info.GetTitle());
  m_subtitle = QString::fromStdString(info.GetSubtitle());
  m_address = QString::fromStdString(info.GetSecondarySubtitle());
  m_secondaryTitle = QString::fromStdString(info.GetSecondaryTitle());
  m_apiBackUrl = info.GetApiUrl().empty() ? QString() : QString::fromStdString(m_framework.GetParsedBackUrl());
  m_osmDescription = QString::fromStdString(info.GetOSMDescription());
  m_isRoutePoint = info.IsRoutePoint();
  // Steps and gates have no routing option.
  switch (info.GetRoadType())
  {
  case RoadWarningMarkType::Toll: m_roadToAvoid = routing::RoutingOptions::Toll; break;
  case RoadWarningMarkType::Ferry: m_roadToAvoid = routing::RoutingOptions::Ferry; break;
  case RoadWarningMarkType::Dirty: m_roadToAvoid = routing::RoutingOptions::Dirty; break;
  default: m_roadToAvoid = 0; break;
  }
  m_countryId = info.IsMyPosition() ? std::string() : info.GetCountryId();
  UpdateCountry();
  m_trackCandidates.clear();
  if (auto const & candidates = info.GetTrackCandidates(); candidates.size() > 1)
  {
    for (auto const & candidate : candidates)
    {
      bool const selected = info.IsRelationTrack() ? candidate.m_relationId == info.GetTrackRelationId()
                                                   : candidate.m_trackId == info.GetTrackId();
      m_trackCandidates.append(QVariantMap{{"title", QString::fromStdString(candidate.m_title)},
                                           {"color", ColorName(candidate.m_color)},
                                           {"selected", selected}});
    }
  }

  auto const entries = place_page::GetAvailableCoordinateFormats(info.GetLatLon(), info.GetCountryId());
  auto const format = place_page::EffectiveCoordinateFormat(entries, SavedCoordinatesFormat());
  m_coordinateValues.clear();
  for (auto const & entry : entries)
  {
    if (entry.m_format == format)
      m_coordinates = QString::fromStdString(entry.m_display);
    m_coordinateValues.append(QString::fromStdString(entry.m_value));
  }

  m_isBookmark = info.IsBookmark();
  m_isTrack = info.IsTrack();
  m_isRelationTrack = m_isTrack && info.IsRelationTrack();
  m_userMarkId = m_isTrack ? info.GetTrackId() : info.GetBookmarkId();
  m_canEdit = info.ShouldShowEditPlace();
  m_canAddPlace = info.ShouldShowAddPlace();
  m_canAddBusiness = info.ShouldShowAddBusiness();
  m_editable = info.CanEditPlace();
  // Tracks are shared as files and have no single point to open elsewhere.
  m_shareText.clear();
  m_geoUri.clear();
  if (!m_isTrack)
  {
    m_shareText = QString::fromStdString(m_framework.GetShareData(info).m_text);
    m_geoUri = QString::fromStdString(ge0::GenerateGeoUri(info.GetLatLon().m_lat, info.GetLatLon().m_lon,
                                                          m_framework.GetDrawScale(), info.GetTitle()));
  }
  UpdateOpeningHours(info.GetOpeningHours());
  m_wikiDescription = QString::fromStdString(info.GetWikiDescription());
  auto const wikipedia = info.GetMetadata(Metadata::FMD_WIKIPEDIA);
  m_wikiUrl = wikipedia.empty() ? QString() : QString::fromStdString(Metadata::ToWikiURL(std::string(wikipedia)));

  m_details.clear();
  auto const add = [this](QString const & icon, QString const & text, QString const & url = {})
  {
    if (!text.isEmpty())
      m_details.append(QVariantMap{{"icon", icon}, {"text", text}, {"url", url}});
  };
  // Same order as the Android place page.
  add("editor/ic_cuisine.svg", QString::fromStdString(info.FormatCuisines()));
  add("placepage/ic_entrance.webp", ToQString(info.GetMetadata(Metadata::FMD_FLATS)));
  if (auto const op = info.GetMetadata(Metadata::FMD_OPERATOR); !op.empty())
    add("image://theme/icon-m-person", Localized("operator", {ToQString(op)}));
  if (auto const network = info.GetMetadata(Metadata::FMD_NETWORK); !network.empty())
    add("placepage/ic_network_white.svg", Localized("network", {ToQString(network)}));

  auto const addWebsite = [&](char const * icon, Metadata::EType type, QString const & text = {})
  {
    QString const url = QUrl::fromPercentEncoding(ToQString(info.GetMetadata(type)).toUtf8());
    if (url.isEmpty())
      return;
    static QRegularExpression const kSchemeAndTrailingSlash(QStringLiteral("^https?://|/$"));
    add(icon, text.isEmpty() ? QString(url).remove(kSchemeAndTrailingSlash) : text,
        url.contains("://") ? url : "https://" + url);
  };
  addWebsite("image://theme/icon-m-website", Metadata::FMD_WEBSITE);
  addWebsite("image://theme/icon-m-website", Metadata::FMD_HERITAGE_WEBSITE);
  addWebsite("editor/ic_website_menu.svg", Metadata::FMD_WEBSITE_MENU, Localized("view_menu"));
  for (auto const & phone : ToQString(info.GetMetadata(Metadata::FMD_PHONE_NUMBER)).split(';', QString::SkipEmptyParts))
    add("image://theme/icon-m-phone", phone.trimmed(), "tel:" + phone.trimmed());
  if (auto const email = ToQString(info.GetMetadata(Metadata::FMD_EMAIL)); !email.isEmpty())
    add("image://theme/icon-m-mail", email, "mailto:" + email);
  std::pair<Metadata::EType, char const *> constexpr kSocial[] = {{Metadata::FMD_CONTACT_FACEBOOK, "ic_facebook.svg"},
                                                                  {Metadata::FMD_CONTACT_INSTAGRAM, "ic_instagram.svg"},
                                                                  {Metadata::FMD_CONTACT_TWITTER, "ic_twitterx.svg"},
                                                                  {Metadata::FMD_CONTACT_VK, "ic_vk.svg"},
                                                                  {Metadata::FMD_CONTACT_LINE, "ic_line.svg"}};
  for (auto const & [type, icon] : kSocial)
  {
    auto const value = info.GetMetadata(type);
    if (!value.empty())
      add(QStringLiteral("editor/%1").arg(icon), ToQString(value),
          QString::fromStdString(osm::socialContactToURL(type, value)));
  }
  if (auto const commons = info.GetMetadata(Metadata::FMD_WIKIMEDIA_COMMONS); !commons.empty())
  {
    add("placepage/ic_wikimedia_commons.svg", Localized("wikimedia_commons"),
        QString::fromStdString(Metadata::ToWikimediaCommonsURL(std::string(commons))));
  }

  if (auto const level = info.GetMetadata(Metadata::FMD_LEVEL); !level.empty())
    add("image://theme/icon-m-levels", Localized("level_value_generic", {ToQString(level)}));
  if (auto const capacity = info.GetMetadata(Metadata::FMD_CAPACITY); !capacity.empty())
    add("placepage/ic_capacity_white.svg", Localized("capacity", {ToQString(capacity)}));
  // Values map to classificator types like wheelchair-yes.
  if (auto const wheelchair = info.GetMetadata(Metadata::FMD_WHEELCHAIR); !wheelchair.empty())
    add("placepage/ic_wheelchair_white.svg",
        QString::fromStdString(platform::GetLocalizedTypeName(std::string(wheelchair))));
  if (auto const internet = info.GetInternet(); internet != feature::Internet::Unknown)
    add("image://theme/icon-m-wlan", Localized(internet == feature::Internet::No ? "no_available" : "yes_available"));
  // An ATM inside a bank or a shop.
  if (feature::HasAtm(info.GetTypes()))
    add("categories/ic_category_atm.svg", QString::fromStdString(platform::GetLocalizedTypeName("amenity-atm")));
  if (info.GetMetadata(Metadata::FMD_DRIVE_THROUGH) == "yes")
    add("editor/ic_drive_through_white.svg", Localized("drive_through"));
  if (auto const selfService = info.GetMetadata(Metadata::FMD_SELF_SERVICE); !selfService.empty())
    add("editor/ic_self_service.svg",
        QString::fromStdString(platform::GetLocalizedTypeName("self_service-" + std::string(selfService))));
  if (info.GetMetadata(Metadata::FMD_OUTDOOR_SEATING) == "yes")
    add("editor/ic_outdoor_seating.svg", Localized("outdoor_seating"));

  m_routes.clear();
  m_routeIds.clear();
  for (auto const & route : info.GetRoutes())
  {
    QString label = QString::fromStdString(route.m_ref);
    if (!route.m_from.empty() || !route.m_to.empty())
    {
      label += ": " + QString::fromStdString(route.m_from);
      if (!route.m_to.empty())
        label += " → " + QString::fromStdString(route.m_to);
    }
    uint32_t const argb = route.m_color.GetARGB();
    // No alpha means the relation has no color.
    QString const color = (argb >> 24) == 0 ? QString() : QColor::fromRgba(argb).name();
    m_routes.append(QVariantMap{{"label", label}, {"color", color}});
    m_routeIds.push_back(route.m_relID);
  }
  static uint32_t const kTramStop = classif().GetTypeByPath({"railway", "tram_stop"});
  m_isTramStop = info.GetTypes().Has(kTramStop);
  UpdateRouteRefs();

  UpdateTrack();
  UpdateList();
  UpdateDistance();
  emit changed();
}

void PlacePage::UpdateList()
{
  m_listName.clear();
  m_listId = kml::kInvalidMarkGroupId;
  m_color.clear();
  m_notes.clear();
  auto const & manager = m_framework.GetBookmarkManager();
  if (m_isBookmark)
  {
    auto const * bookmark = manager.GetBookmark(m_userMarkId);
    if (!bookmark)
      return;
    m_listId = bookmark->GetGroupId();
    m_listName = QString::fromStdString(manager.GetCategoryName(m_listId));
    m_color = ColorName(bookmark->GetColorForRendering());
    m_notes = QString::fromStdString(bookmark->GetDescription());
  }
  else if (m_isTrack)
  {
    auto const * track = manager.GetTrack(m_userMarkId);
    // Public transport route tracks belong to no list.
    if (!track || track->GetGroupId() == kml::kInvalidMarkGroupId)
      return;
    m_listId = track->GetGroupId();
    m_listName = QString::fromStdString(manager.GetCategoryName(m_listId));
    m_color = ColorName(track->GetColor(0));
    m_notes = QString::fromStdString(kml::GetDefaultStr(track->GetData().m_description));
  }
}

void PlacePage::UpdateCountry()
{
  // Outdated maps are offered for update too.
  auto country = MissingMapInfo(m_framework.GetStorage(), m_countryId, true /* withOutdated */);
  if (country != m_country)
  {
    m_country = std::move(country);
    emit countryChanged();
  }
}

void PlacePage::setList(quint64 listId)
{
  ASSERT_NOT_EQUAL(m_listId, kml::kInvalidMarkGroupId, ());
  {
    auto session = m_framework.GetBookmarkManager().GetEditSession();
    if (m_isTrack)
      session.MoveTrack(m_userMarkId, m_listId, listId);
    else
      session.MoveBookmark(m_userMarkId, m_listId, listId);
  }
  m_framework.UpdatePlacePageInfoForCurrentSelection();
}

void PlacePage::setColor(int colorIndex)
{
  auto const color = PresetColor(colorIndex);
  {
    auto session = m_framework.GetBookmarkManager().GetEditSession();
    if (m_isTrack)
      session.ChangeTrackColor(m_userMarkId, color);
    else
      session.SetBookmarksAndTracksColor({m_userMarkId}, {}, color);
  }
  m_framework.UpdatePlacePageInfoForCurrentSelection();
}

void PlacePage::selectTrackCandidate(int index)
{
  if (!m_framework.HasPlacePageInfo())
    return;
  auto const & candidates = m_framework.GetCurrentPlacePageInfo().GetTrackCandidates();
  ASSERT(index >= 0 && static_cast<size_t>(index) < candidates.size(), (index));
  auto const & candidate = candidates[static_cast<size_t>(index)];
  m_framework.SelectTrackCandidate(candidate.m_trackId, candidate.m_relationId);
}

void PlacePage::UpdateMyPosition(bool hasAltitude, double altitude, double speed)
{
  if (!m_framework.HasPlacePageInfo() || !m_framework.GetCurrentPlacePageInfo().IsMyPosition())
    return;
  QStringList parts;
  if (hasAltitude)
    parts.append(FormatAltitude(altitude));
  if (speed >= 0)
    parts.append(FormatSpeed(speed));
  QString const subtitle = JoinDetails(parts);
  if (subtitle != m_subtitle)
  {
    m_subtitle = subtitle;
    emit changed();
  }
}

void PlacePage::UpdateRouteRefs()
{
  // Both directions of a line share a ref.
  QStringList seen;
  QStringList refs;
  QString const active = QString::fromStdString(m_framework.GetActiveTransitRouteRef());
  if (m_framework.HasPlacePageInfo())
  {
    for (auto const & route : m_framework.GetCurrentPlacePageInfo().GetRoutes())
    {
      QString const ref = QString::fromStdString(route.m_ref);
      if (seen.contains(ref))
        continue;
      seen.append(ref);
      refs.append(ref == active ? "<b><u>" + ref.toHtmlEscaped() + "</u></b>" : ref.toHtmlEscaped());
    }
  }
  m_routeRefs = JoinDetails(refs);
}

void PlacePage::showRoute(int index)
{
  ASSERT(index >= 0 && index < static_cast<int>(m_routeIds.size()), (index));
  m_framework.ShowRouteTransit(m_routeIds[static_cast<size_t>(index)]);
  UpdateRouteRefs();
  emit changed();
}

void PlacePage::UpdateTrack()
{
  m_trackStats.clear();
  m_elevation.clear();
  auto & manager = m_framework.GetBookmarkManager();
  auto const * track = m_isTrack ? manager.GetTrack(m_userMarkId) : nullptr;
  if (!track)
    return;

  auto const add = [this](char const * label, QString const & value)
  { m_trackStats.append(QVariantMap{{"label", Localized(label)}, {"value", value}}); };
  auto const stats = track->GetStatistics();
  add("elevation_profile_distance", QString::fromStdString(stats.GetFormattedLength()));
  if (stats.m_duration > 0)
    add("elevation_profile_time", FormatDuration(static_cast<long>(stats.m_duration)));

  auto const * elevation = track->GetElevationInfo();
  if (!elevation || elevation->IsEmpty())
    return;
  m_elevation = ElevationChart(*elevation, stats.m_minElevation, stats.m_maxElevation);
  add("elevation_profile_ascent", QString::fromStdString(stats.GetFormattedAscent()));
  add("elevation_profile_descent", QString::fromStdString(stats.GetFormattedDescent()));
  add("elevation_profile_max_elevation", m_elevation["max"].toString());
  add("elevation_profile_min_elevation", m_elevation["min"].toString());
  char const * const difficulties[] = {nullptr, "elevation_profile_diff_level_easy",
                                       "elevation_profile_diff_level_moderate", "elevation_profile_diff_level_hard"};
  if (auto const difficulty = elevation->GetDifficulty(); difficulty > 0 && difficulty < std::size(difficulties))
    add("elevation_profile_difficulty", Localized(difficulties[difficulty]));

  // The position marker follows location updates along the track.
  manager.UpdateElevationMyPosition(m_userMarkId);
}

void PlacePage::UpdateOpeningHours(std::string_view openingHours)
{
  m_openingHours = ToQString(openingHours);
  m_openingSchedule.clear();
  m_openState = OpenUnknown;
  m_openTitle.clear();
  m_openDescription.clear();

  if (openingHours.empty())
    return;
  osmoh::OpeningHours const oh(openingHours);
  if (!oh.IsValid())
    return;

  editor::ui::TimeTableSet tts;
  if (!oh.IsTwentyFourHours() && editor::MakeTimeTableSet(oh, tts))
    m_openingSchedule = MakeWeekSchedule(tts);

  // Local time; the feature time zone is ignored.
  time_t const now = std::time(nullptr);
  auto const info = oh.GetInfo(now);
  if (info.state == osmoh::RuleState::Unknown)
    return;

  QLocale const locale = QLocale::system();
  auto const time = [](time_t t) { return FormatTime(QDateTime::fromTime_t(static_cast<uint>(t)).time()); };

  if (oh.IsTwentyFourHours())
  {
    m_openState = Open;
    m_openTitle = Localized("twentyfour_seven");
    return;
  }

  // Thresholds and wording follow Android.
  if (info.state == osmoh::RuleState::Open)
  {
    m_openState = Open;
    m_openTitle = Localized("editor_time_open");
    if (info.nextTimeClosed <= now)
      return;
    long const minutes = (info.nextTimeClosed - now) / 60;
    if (minutes < 3 * 60)
      m_openDescription =
          JoinDetails({Localized("closes_in", {FormatDuration(info.nextTimeClosed - now)}), time(info.nextTimeClosed)});
    else if (minutes < 24 * 60)
      m_openDescription = Localized("closes_at", {time(info.nextTimeClosed)});
    return;
  }

  m_openState = Closed;
  m_openTitle = Localized("closed_now");
  if (info.nextTimeOpen <= now)
    return;
  long const minutes = (info.nextTimeOpen - now) / 60;
  QDateTime const opens = QDateTime::fromTime_t(static_cast<uint>(info.nextTimeOpen));
  if (minutes < 3 * 60)
    m_openDescription =
        JoinDetails({Localized("opens_in", {FormatDuration(info.nextTimeOpen - now)}), time(info.nextTimeOpen)});
  else if (opens.date() == QDate::currentDate())
    m_openDescription = Localized("opens_at", {time(info.nextTimeOpen)});
  else if (minutes < 24 * 60)
    m_openDescription = Localized("opens_tomorrow_at", {time(info.nextTimeOpen)});
  else if (minutes < 7 * 24 * 60)
    m_openDescription =
        Localized("opens_dayoftheweek_at", {locale.dayName(opens.date().dayOfWeek()), time(info.nextTimeOpen)});
}

void PlacePage::SetNorth(double north)
{
  m_north = north;
  if (m_open)
    UpdateDistance();
}

void PlacePage::UpdateDistance()
{
  QString distance;
  double azimuth = -1.0;
  QString bearing;
  if (m_framework.HasPlacePageInfo())
  {
    if (auto const position = m_framework.GetCurrentPosition())
    {
      // Tracks have no single point to head to.
      auto const & info = m_framework.GetCurrentPlacePageInfo();
      if (!info.IsMyPosition() && !info.IsTrack())
      {
        auto const ll = mercator::ToLatLon(*position);
        platform::Distance d;
        double azimut;
        m_framework.GetDistanceAndAzimut(info.GetMercator(), ll.m_lat, ll.m_lon, m_north, d, azimut);
        distance = QString::fromStdString(d.ToString());
        azimuth = math::RadToDeg(azimut);
        bearing = QString::number(std::lround(math::RadToDeg(ang::AngleIn2PI(azimut + m_north)))) + "°";
      }
    }
  }
  if (distance != m_distance || azimuth != m_azimuth || bearing != m_bearing)
  {
    m_distance = distance;
    m_azimuth = azimuth;
    m_bearing = bearing;
    emit distanceChanged();
  }
}
}  // namespace sailfish
