#include "qt/place_page_dialog_developer.hpp"
#include "qt/place_page_dialog_common.hpp"

#include "qt/qt_common/text_dialog.hpp"

#include "map/place_page_info.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <format>
#include <string>

// TODO: remove
#include <glaze/json/read.hpp>
#include <sstream>
#include "coding/url.hpp"
#include "pt_private/quad_tree_encoder.h"

#include "platform/http_client.hpp"

struct Stop
{
  std::string_view id;
  std::string_view stop_name;
  std::vector<double> lat_lon;  // [latitude, longitude]
};

struct Route
{
  std::string_view routeId;
  std::string_view shortName;
  std::string_view longName;
  std::string_view routeType;
  std::string_view typeRaw;
  std::string_view agency;
};

struct Calendar
{
  std::string_view serviceId;
  std::string_view week;
  int periodStart;
  int periodEnd;
  std::optional<std::vector<int>> datesIncluded;
  std::vector<int> datesExcluded;
};

struct StopOnRoutePosition
{
  int sequence;
  std::optional<std::string_view> nextStopId;
  std::optional<std::string_view> nextStopName;
  std::optional<std::string_view> prevStopId;
  std::optional<std::string_view> prevStopName;
  std::string_view firstStopId;
  std::string_view firstStopName;
  std::string_view lastStopId;
  std::string_view lastStopName;
};

struct RouteStopTime
{
  StopOnRoutePosition stopOnRoutePosition;
  std::string_view timezone;
  std::string_view route;
  std::vector<std::string_view> tripIds;
  std::vector<int> arrivalTimes;    // Time in seconds
  std::vector<int> departureTimes;  // Time in seconds
};

struct Section
{
  Calendar calendar;
  std::vector<RouteStopTime> routeStopTimes;
};

struct Schedule
{
  Stop stop;
  std::map<std::string_view, Route> routes;
  std::vector<Section> sections;
};

namespace
{
constexpr std::string_view daysOfWeek[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

// Returns multiline string visible to user.
std::string FormatSchedule(std::string const & jsonSchedule)
{
  std::ostringstream s;

  auto const schedules = glz::read_json<std::vector<Schedule>>(jsonSchedule);
  if (!schedules)
  {
    s << "Failed to parse schedule: " << schedules.error();
    return s.str();
  }

  for (Schedule const & schedule : *schedules)
  {
    s << schedule.stop.stop_name << " (" << schedule.stop.id << ")" << '\n';

    auto const now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm const * local_time = std::localtime(&now_time_t);

    int year = local_time->tm_year + 1900;
    int month = local_time->tm_mon + 1;
    int day = local_time->tm_mday;

    std::string_view dow = daysOfWeek[local_time->tm_wday];

    int i_date = year * 10000 + month * 100 + day;

    // Find today's section
    for (auto const & section : schedule.sections)
    {
      Calendar const & cal = section.calendar;
      if (cal.periodStart > i_date || cal.periodEnd < i_date)
        continue;

      if (!cal.datesExcluded.empty())
      {
        auto const & excluded = cal.datesExcluded;
        if (std::find(excluded.begin(), excluded.end(), i_date) != excluded.end())
          continue;
      }

      if (cal.week.find(dow) != std::string::npos ||
          (cal.datesIncluded && std::ranges::find(*cal.datesIncluded, i_date) != cal.datesIncluded->end()))
      {
        s << cal.week << " (" << cal.periodStart << " " << cal.periodEnd << ")" << '\n';
        for (auto const & rs : section.routeStopTimes)
          if (auto const r = schedule.routes.find(rs.route); r != schedule.routes.end())
            s << r->second.shortName << ';';
      }
    }
  }
  return s.str();
}
}  // namespace

PlacePageDialogDeveloper::PlacePageDialogDeveloper(QWidget * parent, place_page::Info const & info) : QDialog(parent)
{
  QVBoxLayout * layout = new QVBoxLayout();
  QGridLayout * grid = new QGridLayout();
  int row = 0;

  /// @todo Many dupicates with PlacePageDialogUser. Factor out some base class.
  auto const addEntry = [grid, &row](std::string const & key, std::string const & value, bool isLink = false)
  {
    grid->addWidget(new QLabel(QString::fromStdString(key)), row, 0);
    QLabel * label = new QLabel(QString::fromStdString(value));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (isLink)
    {
      label->setOpenExternalLinks(true);
      label->setTextInteractionFlags(Qt::TextBrowserInteraction);
      label->setText(QString::fromStdString("<a href=\"" + value + "\">" + value + "</a>"));
    }
    grid->addWidget(label, row++, 1);
    return label;
  };

  {
    ms::LatLon const ll = info.GetLatLon();
    addEntry("lat, lon", strings::to_string_dac(ll.m_lat, 7) + ", " + strings::to_string_dac(ll.m_lon, 7));
  }

  addEntry("CountryId", info.GetCountryId());

  auto const & title = info.GetTitle();
  if (!title.empty())
    addEntry("Title", title);

  if (auto const & subTitle = info.GetSubtitle(); !subTitle.empty())
    addEntry("Subtitle", subTitle);

  addEntry("Address", info.GetAddress());

  if (info.IsBookmark())
  {
    grid->addWidget(new QLabel("Bookmark"), row, 0);
    grid->addWidget(new QLabel("Yes"), row++, 1);
  }

  if (info.IsMyPosition())
  {
    grid->addWidget(new QLabel("MyPosition"), row, 0);
    grid->addWidget(new QLabel("Yes"), row++, 1);
  }

  if (info.HasApiUrl())
  {
    grid->addWidget(new QLabel("Api URL"), row, 0);
    grid->addWidget(new QLabel(QString::fromStdString(info.GetApiUrl())), row++, 1);
  }

  if (info.IsFeature())
  {
    addEntry("Feature ID", DebugPrint(info.GetID()));
    addEntry("Raw Types", DebugPrint(info.GetTypes()));
  }

  auto const layer = info.GetLayer();
  if (layer != feature::LAYER_EMPTY)
    addEntry("Layer", std::to_string(layer));

  using PropID = osm::MapObject::MetadataID;

  // Route refs
  if (auto routes = info.FormatRouteRefs(); !routes.empty())
    addEntry("Routes", routes);

  if (info.HasOnlineSchedule())
  {
    auto const [lat, lon] = info.GetLatLon();

    // auto const id = info.GetMetadata(PropID::FMD_SCHEDULE_ID);
    auto const id = QuadTreeEncoder::LatLonToBase62(lat, lon);
    // TODO: Params after ? are for debugging purposes. Remove.
    auto url = std::format("http://osm.me:4567/v1/schedule/{}?lat={:.8f}&lon={:.8f}&name={}&types={}", id, lat, lon,
                           url::UrlEncode(info.GetPrimaryFeatureName()),  // Likely translated on mobiles.
                           url::UrlEncode(strings::JoinStrings(info.GetRawTypes(), ';')));
    if (auto const localRef = info.GetMetadata(PropID::FMD_LOCAL_REF); !localRef.empty())
      url += "&local_ref=" + url::UrlEncode(localRef);

    QLabel * scheduleLabel = addEntry("Schedule", "Loading...");
    GetPlatform().RunTask(Platform::Thread::Network, [url = std::move(url), scheduleLabel]()
    {
      platform::HttpClient client{url};
      std::string uiString;
      if (client.RunHttpRequest())
        uiString = FormatSchedule(client.ServerResponse());
      else
        uiString = "Failed to connect to " + url::UrlDecode(client.UrlRequested());

      auto qs = QString::fromStdString(uiString);
      scheduleLabel->setText(qs);
    });
  }

  // Opening hours fragment
  if (auto openingHours = info.GetOpeningHours(); !openingHours.empty())
    addEntry(DebugPrint(PropID::FMD_OPEN_HOURS), std::string(openingHours));

  // Cuisine fragment
  if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
    addEntry(DebugPrint(PropID::FMD_CUISINE), cuisines);

  layout->addLayout(grid);

  QDialogButtonBox * dbb = new QDialogButtonBox();
  place_page_dialog::addCommonButtons(this, dbb, info.ShouldShowEditPlace());

  if (auto const & descr = info.GetWikiDescription(); !descr.empty())
  {
    QPushButton * wikiButton = new QPushButton("Wiki Description");
    connect(wikiButton, &QAbstractButton::clicked, this, [this, descr, title]()
    {
      auto textDialog = TextDialog(this, QString::fromStdString(descr), QString::fromStdString("Wikipedia: " + title));
      textDialog.exec();
    });
    dbb->addButton(wikiButton, QDialogButtonBox::ActionRole);
  }

  info.ForEachMetadataReadable([&addEntry](PropID id, std::string const & value)
  {
    bool isLink = false;
    switch (id)
    {
    case PropID::FMD_EMAIL:
    case PropID::FMD_WEBSITE:
    case PropID::FMD_CONTACT_FACEBOOK:
    case PropID::FMD_CONTACT_INSTAGRAM:
    case PropID::FMD_CONTACT_TWITTER:
    case PropID::FMD_CONTACT_VK:
    case PropID::FMD_CONTACT_LINE:
    case PropID::FMD_WIKIPEDIA:
    case PropID::FMD_WIKIMEDIA_COMMONS: isLink = true; break;
    default: break;
    }

    addEntry(DebugPrint(id), value, isLink);
  });

  layout->addWidget(dbb);
  setLayout(layout);

  auto const ppTitle = std::string("Place Page") + (info.IsBookmark() ? " (bookmarked)" : "");
  setWindowTitle(ppTitle.c_str());
}
