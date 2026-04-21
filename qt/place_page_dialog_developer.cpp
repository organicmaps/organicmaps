#include "qt/place_page_dialog_developer.hpp"
#include "qt/place_page_dialog_common.hpp"

#include "qt/qt_common/text_dialog.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <string>

PlacePageDialogDeveloper::PlacePageDialogDeveloper(QWidget * parent, place_page::Info const & info,
                                                   Framework & framework)
  : QDialog(parent)
  , m_framework(framework)
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
  else if (info.IsTrack() && info.GetTrackId() == kml::kTempRelationTrackId)
  {
    grid->addWidget(new QLabel("Track from Relation"), row, 0);
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

  // Route refs — one clickable button per route; click shows the route's transit view.
  if (auto const & routes = info.GetRoutes(); !routes.empty())
  {
    grid->addWidget(new QLabel("Routes"), row, 0);

    QHBoxLayout * routesRow = new QHBoxLayout();
    routesRow->setContentsMargins(0, 0, 0, 0);
    for (auto const & r : routes)
    {
      QPushButton * btn = new QPushButton(QString::fromStdString(r.m_ref));
      btn->setAutoDefault(false);
      btn->setToolTip(QString::fromStdString(r.m_from + (r.m_to.empty() ? "" : " → " + r.m_to)));
      uint32_t const relID = r.m_relID;
      connect(btn, &QAbstractButton::clicked, this, [this, relID]() { m_framework.ShowRouteTransit(relID); });
      routesRow->addWidget(btn);
    }
    routesRow->addStretch();

    QWidget * routesContainer = new QWidget();
    routesContainer->setLayout(routesRow);
    grid->addWidget(routesContainer, row++, 1);
  }

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

  if (info.IsTrack())
  {
    QPushButton * fromButton = new QPushButton("Route Along Track");
    fromButton->setAutoDefault(false);
    connect(fromButton, &QAbstractButton::clicked, this, [this] { done(place_page_dialog::RouteAlong); });
    dbb->addButton(fromButton, QDialogButtonBox::ActionRole);
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

  std::string ppTitle("Place Page");
  if (info.IsBookmark())
    ppTitle.append(" (Bookmark)");
  else if (info.IsTrack())
    ppTitle.append(" (Track)");
  setWindowTitle(QString::fromStdString(ppTitle));
}
