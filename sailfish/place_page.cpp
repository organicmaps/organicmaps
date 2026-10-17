#include "sailfish/place_page.hpp"

#include "sailfish/countries_model.hpp"
#include "sailfish/helpers.hpp"
#include "sailfish/opening_hours_editor.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include "ge0/url_generator.hpp"

#include "indexer/feature_utils.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

#include "opening_hours/opening_hours.hpp"

#include "storage/storage.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/settings.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

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

void AddDetail(QVariantList & details, QString const & icon, QString const & text, QString const & url = {})
{
  if (!text.isEmpty())
    details.append(QVariantMap{{"icon", icon}, {"text", text}, {"url", url}});
}

void AddWebsite(QVariantList & details, place_page::Info const & info, char const * icon, Metadata::EType type,
                QString const & text = {})
{
  QString const url = ToQString(info.GetMetadata(type));
  if (url.isEmpty())
    return;
  // Decoded only for display: e.g. %26 in a query must stay encoded for the link.
  static QRegularExpression const kSchemeAndTrailingSlash(QStringLiteral("^https?://|/$"));
  AddDetail(details, icon,
            text.isEmpty() ? QUrl::fromPercentEncoding(url.toUtf8()).remove(kSchemeAndTrailingSlash) : text,
            url.contains("://") ? url : "https://" + url);
}

void AddContactDetails(QVariantList & details, place_page::Info const & info)
{
  AddWebsite(details, info, "image://theme/icon-m-website", Metadata::FMD_WEBSITE);
  AddWebsite(details, info, "image://theme/icon-m-website", Metadata::FMD_HERITAGE_WEBSITE);
  AddWebsite(details, info, "editor/ic_website_menu.svg", Metadata::FMD_WEBSITE_MENU, Localized("view_menu"));
  for (auto const & phone : ToQString(info.GetMetadata(Metadata::FMD_PHONE_NUMBER)).split(';', QString::SkipEmptyParts))
    AddDetail(details, "image://theme/icon-m-phone", phone.trimmed(), "tel:" + phone.trimmed());
  if (auto const email = ToQString(info.GetMetadata(Metadata::FMD_EMAIL)); !email.isEmpty())
    AddDetail(details, "image://theme/icon-m-mail", email, "mailto:" + email);
  std::pair<Metadata::EType, char const *> constexpr kSocial[] = {{Metadata::FMD_CONTACT_FACEBOOK, "ic_facebook.svg"},
                                                                  {Metadata::FMD_CONTACT_INSTAGRAM, "ic_instagram.svg"},
                                                                  {Metadata::FMD_CONTACT_TWITTER, "ic_twitterx.svg"},
                                                                  {Metadata::FMD_CONTACT_VK, "ic_vk.svg"},
                                                                  {Metadata::FMD_CONTACT_LINE, "ic_line.svg"}};
  for (auto const & [type, icon] : kSocial)
  {
    auto const value = info.GetMetadata(type);
    if (!value.empty())
      AddDetail(details, QStringLiteral("editor/%1").arg(icon), ToQString(value),
                QString::fromStdString(osm::socialContactToURL(type, value)));
  }
  if (auto const commons = info.GetMetadata(Metadata::FMD_WIKIMEDIA_COMMONS); !commons.empty())
  {
    AddDetail(details, "placepage/ic_wikimedia_commons.svg", Localized("wikimedia_commons"),
              QString::fromStdString(Metadata::ToWikimediaCommonsURL(std::string(commons))));
  }
}

void AddAmenityDetails(QVariantList & details, place_page::Info const & info)
{
  if (auto const level = info.GetMetadata(Metadata::FMD_LEVEL); !level.empty())
    AddDetail(details, "image://theme/icon-m-levels", Localized("level_value_generic", {ToQString(level)}));
  if (auto const capacity = info.GetMetadata(Metadata::FMD_CAPACITY); !capacity.empty())
    AddDetail(details, "placepage/ic_capacity_white.svg", Localized("capacity", {ToQString(capacity)}));
  // Values map to classificator types like wheelchair-yes.
  if (auto const wheelchair = info.GetMetadata(Metadata::FMD_WHEELCHAIR); !wheelchair.empty())
    AddDetail(details, "placepage/ic_wheelchair_white.svg",
              QString::fromStdString(platform::GetLocalizedTypeName(std::string(wheelchair))));
  if (auto const internet = info.GetInternet(); internet != feature::Internet::Unknown)
    AddDetail(details, "image://theme/icon-m-wlan",
              Localized(internet == feature::Internet::No ? "no_available" : "yes_available"));
  // An ATM inside a bank or a shop.
  if (feature::HasAtm(info.GetTypes()))
    AddDetail(details, "categories/ic_category_atm.svg",
              QString::fromStdString(platform::GetLocalizedTypeName("amenity-atm")));
  if (info.GetMetadata(Metadata::FMD_DRIVE_THROUGH) == "yes")
    AddDetail(details, "editor/ic_drive_through_white.svg", Localized("drive_through"));
  if (auto const selfService = info.GetMetadata(Metadata::FMD_SELF_SERVICE); !selfService.empty())
    AddDetail(details, "editor/ic_self_service.svg",
              QString::fromStdString(platform::GetLocalizedTypeName("self_service-" + std::string(selfService))));
  if (info.GetMetadata(Metadata::FMD_OUTDOOR_SEATING) == "yes")
    AddDetail(details, "editor/ic_outdoor_seating.svg", Localized("outdoor_seating"));
}

// Same order as the Android place page.
QVariantList MakeDetails(place_page::Info const & info)
{
  QVariantList details;
  AddDetail(details, "editor/ic_cuisine.svg", QString::fromStdString(info.FormatCuisines()));
  AddDetail(details, "placepage/ic_entrance.webp", ToQString(info.GetMetadata(Metadata::FMD_FLATS)));
  if (auto const op = info.GetMetadata(Metadata::FMD_OPERATOR); !op.empty())
    AddDetail(details, "image://theme/icon-m-person", Localized("operator", {ToQString(op)}));
  if (auto const network = info.GetMetadata(Metadata::FMD_NETWORK); !network.empty())
    AddDetail(details, "placepage/ic_network_white.svg", Localized("network", {ToQString(network)}));
  AddContactDetails(details, info);
  AddAmenityDetails(details, info);
  return details;
}

QString FormatTimeT(time_t t)
{
  return FormatTime(QDateTime::fromTime_t(static_cast<uint>(t)).time());
}

// Thresholds and wording follow Android.
QString ClosesDescription(time_t now, time_t nextTimeClosed)
{
  if (nextTimeClosed <= now)
    return {};
  long const minutes = (nextTimeClosed - now) / 60;
  if (minutes < 3 * 60)
    return JoinDetails({Localized("closes_in", {FormatDuration(nextTimeClosed - now)}), FormatTimeT(nextTimeClosed)});
  if (minutes < 24 * 60)
    return Localized("closes_at", {FormatTimeT(nextTimeClosed)});
  return {};
}

QString OpensDescription(time_t now, time_t nextTimeOpen)
{
  if (nextTimeOpen <= now)
    return {};
  long const minutes = (nextTimeOpen - now) / 60;
  QDateTime const opens = QDateTime::fromTime_t(static_cast<uint>(nextTimeOpen));
  if (minutes < 3 * 60)
    return JoinDetails({Localized("opens_in", {FormatDuration(nextTimeOpen - now)}), FormatTimeT(nextTimeOpen)});
  if (opens.date() == QDate::currentDate())
    return Localized("opens_at", {FormatTimeT(nextTimeOpen)});
  if (minutes < 24 * 60)
    return Localized("opens_tomorrow_at", {FormatTimeT(nextTimeOpen)});
  if (minutes < 7 * 24 * 60)
    return Localized("opens_dayoftheweek_at",
                     {QLocale::system().dayName(opens.date().dayOfWeek()), FormatTimeT(nextTimeOpen)});
  return {};
}
}  // namespace

PlacePage::PlacePage(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  m_framework.SetPlacePageListeners(
      [this]
  {
    Update();
    emit selected();
  }, [this]
  {
    m_open = false;
    emit changed();
  }, [this] { Update(); }, [this] { emit switchFullScreen(); });
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
  m_countryId = info.IsMyPosition() ? std::string() : info.GetCountryId();
  UpdateCountry();
  UpdateCoordinates(info);

  m_shareText = QString::fromStdString(m_framework.GetShareData(info).m_text);
  m_geoUri = QString::fromStdString(
      ge0::GenerateGeoUri(info.GetLatLon().m_lat, info.GetLatLon().m_lon, m_framework.GetDrawScale(), info.GetTitle()));
  UpdateOpeningHours(info.GetOpeningHours());
  m_wikiDescription = UserStyledText(info.GetWikiDescription());
  auto const wikipedia = info.GetMetadata(Metadata::FMD_WIKIPEDIA);
  m_wikiUrl = wikipedia.empty() ? QString() : QString::fromStdString(Metadata::ToWikiURL(std::string(wikipedia)));
  m_details = MakeDetails(info);
  UpdateDistance();
  emit changed();
}

void PlacePage::UpdateCoordinates(place_page::Info const & info)
{
  auto const entries = place_page::GetAvailableCoordinateFormats(info.GetLatLon(), info.GetCountryId());
  auto const format = place_page::EffectiveCoordinateFormat(entries, SavedCoordinatesFormat());
  m_coordinateValues.clear();
  for (auto const & entry : entries)
  {
    if (entry.m_format == format)
      m_coordinates = QString::fromStdString(entry.m_display);
    m_coordinateValues.append(QString::fromStdString(entry.m_value));
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

  if (oh.IsTwentyFourHours())
  {
    m_openState = Open;
    m_openTitle = Localized("twentyfour_seven");
    return;
  }

  if (info.state == osmoh::RuleState::Open)
  {
    m_openState = Open;
    m_openTitle = Localized("editor_time_open");
    m_openDescription = ClosesDescription(now, info.nextTimeClosed);
    return;
  }

  m_openState = Closed;
  m_openTitle = Localized("closed_now");
  m_openDescription = OpensDescription(now, info.nextTimeOpen);
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
      auto const & info = m_framework.GetCurrentPlacePageInfo();
      if (!info.IsMyPosition())
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
