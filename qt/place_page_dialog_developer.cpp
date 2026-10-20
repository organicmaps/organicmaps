#include "qt/place_page_dialog_developer.hpp"

#include "qt/draw_widget.hpp"
#include "qt/qt_common/text_dialog.hpp"

#include "map/place_page_info.hpp"

#include "indexer/validate_and_format_contacts.hpp"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <string>

PlacePageDialogDeveloper::PlacePageDialogDeveloper(QWidget * parent, qt::DrawWidget * drawWidget,
                                                   place_page::Info const & info)
  : PlacePageDialogCommon(parent, drawWidget, info)
{
  using namespace place_page_dialog;
  QVBoxLayout * contentLayout = GetContentLayout();

  QGridLayout * grid = new QGridLayout();
  int row = 0;

  {
    ms::LatLon const ll = info.GetLatLon();
    addEntry(grid, row, "lat, lon",
             QString::fromStdString(strings::to_string_dac(ll.m_lat, 7) + ", " + strings::to_string_dac(ll.m_lon, 7)));
  }

  addEntry(grid, row, "CountryId", QString::fromStdString(info.GetCountryId()));

  auto const & title = info.GetTitle();
  if (!title.empty())
    addEntry(grid, row, "Title", QString::fromStdString(title));

  if (auto const & subTitle = info.GetSubtitle(); !subTitle.empty())
    addEntry(grid, row, "Subtitle", QString::fromStdString(subTitle));

  addEntry(grid, row, "Address", QString::fromStdString(info.GetAddress()));

  if (info.IsBookmark())
    addEntry(grid, row, "Bookmark", "Yes");
  else if (info.IsRelationTrack())
    addEntry(grid, row, "Track from Relation", "Yes");

  if (info.IsMyPosition())
    addEntry(grid, row, "MyPosition", "Yes");

  if (info.HasApiUrl())
    addEntry(grid, row, "Api URL", QString::fromStdString(info.GetApiUrl()));

  if (info.IsFeature())
  {
    addEntry(grid, row, "Feature ID", QString::fromStdString(DebugPrint(info.GetID())));
    addEntry(grid, row, "Raw Types", QString::fromStdString(DebugPrint(info.GetTypes())));
  }

  auto const layer = info.GetLayer();
  if (layer != feature::LAYER_EMPTY)
    addEntry(grid, row, "Layer", QString::number(layer));

  using PropID = osm::MapObject::MetadataID;

  addRoutesRow(grid, row, drawWidget, info);

  // Cuisine fragment
  if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
    addEntry(grid, row, QString::fromStdString(DebugPrint(PropID::FMD_CUISINE)), QString::fromStdString(cuisines));

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

  info.ForEachMetadataReadable([grid, &row](PropID id, std::string const & value)
  {
    QString url;
    switch (id)
    {
    case PropID::FMD_EMAIL: url = "mailto:" + QString::fromStdString(value); break;
    case PropID::FMD_WEBSITE:
    case PropID::FMD_HERITAGE_WEBSITE:
    case PropID::FMD_WIKIPEDIA:
    case PropID::FMD_WIKIMEDIA_COMMONS: url = QString::fromStdString(value); break;
    case PropID::FMD_CONTACT_FACEBOOK:
    case PropID::FMD_CONTACT_INSTAGRAM:
    case PropID::FMD_CONTACT_TWITTER:
    case PropID::FMD_CONTACT_VK:
    case PropID::FMD_CONTACT_LINE: url = QString::fromStdString(osm::socialContactToURL(id, value)); break;
    default: break;
    }

    addEntry(grid, row, QString::fromStdString(DebugPrint(id)), QString::fromStdString(value), url);
  });

  contentLayout->addStretch();
}
