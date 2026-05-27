#include "qt/place_page_dialog_developer.hpp"
#include "qt/draw_widget.hpp"
#include "qt/place_page_dialog_common.hpp"

#include "qt/qt_common/text_dialog.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

#include <string>

PlacePageDialogDeveloper::PlacePageDialogDeveloper(QWidget * parent, qt::DrawWidget * drawWidget,
                                                   place_page::Info const & info)
  : QWidget(parent)
  , m_drawWidget(drawWidget)
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(place_page_dialog::createActionToolBar(this, drawWidget, info));

  QScrollArea * scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QWidget * content = new QWidget(scrollArea);
  QVBoxLayout * contentLayout = new QVBoxLayout(content);

  QGridLayout * grid = new QGridLayout();
  int row = 0;

  /// @todo Many dupicates with PlacePageDialogUser. Factor out some base class.
  auto const addEntry = [grid, &row](std::string const & key, std::string const & value, bool isLink = false)
  {
    grid->addWidget(new QLabel(QString::fromStdString(key)), row, 0);
    QLabel * label = new QLabel(QString::fromStdString(value));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
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
  else if (info.IsRelationTrack())
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

  // Route refs — pick a route from the combo to show its transit view.
  if (auto const & routes = info.GetRoutes(); !routes.empty())
  {
    grid->addWidget(new QLabel("Routes"), row, 0);

    QComboBox * routesCombo = new QComboBox();
    // Placeholder so opening the widget does not auto-select (and trigger) the first route.
    routesCombo->setPlaceholderText("Select a route…");
    for (auto const & r : routes)
    {
      QString const text = QString::fromStdString(r.m_ref);
      QString const tip = QString::fromStdString(r.m_from + (r.m_to.empty() ? "" : " → " + r.m_to));
      routesCombo->addItem(text, QVariant::fromValue<uint32_t>(r.m_relID));
      routesCombo->setItemData(routesCombo->count() - 1, tip, Qt::ToolTipRole);
    }
    connect(routesCombo, QOverload<int>::of(&QComboBox::activated), this, [this, routesCombo](int idx)
    { m_drawWidget->GetFramework().ShowRouteTransit(routesCombo->itemData(idx).value<uint32_t>()); });

    grid->addWidget(routesCombo, row++, 1);
  }

  // Cuisine fragment
  if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
    addEntry(DebugPrint(PropID::FMD_CUISINE), cuisines);

  grid->setColumnStretch(0, 0);
  grid->setColumnStretch(1, 1);
  contentLayout->addLayout(grid);

  if (auto const & descr = info.GetWikiDescription(); !descr.empty())
  {
    QPushButton * wikiButton = new QPushButton("Wiki Description");
    wikiButton->setAutoDefault(false);
    connect(wikiButton, &QAbstractButton::clicked, this, [this, descr, title]()
    {
      auto textDialog = TextDialog(this, QString::fromStdString(descr), QString::fromStdString("Wikipedia: " + title));
      textDialog.exec();
    });
    contentLayout->addWidget(wikiButton);
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

  contentLayout->addStretch();
  scrollArea->setWidget(content);
  layout->addWidget(scrollArea);
}
