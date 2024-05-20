#include "qt/place_page_dialog_common.hpp"
#include "qt/place_page_dialog_developer.hpp"

#include "qt/qt_common/text_dialog.hpp"

#include "map/place_page_info.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <string>

PlacePageDialogDeveloper::PlacePageDialogDeveloper(QWidget * parent, place_page::Info const & info,
                                                   search::ReverseGeocoder::Address const & address)
  : QDialog(parent)
{
  QVBoxLayout * layout = new QVBoxLayout();
  QGridLayout * grid = new QGridLayout();
  int row = 0;

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

  addEntry("Address", address.FormatAddress());

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
    case PropID::FMD_WIKIMEDIA_COMMONS:
      isLink = true;
      break;
    default:
      break;
    }

    addEntry(DebugPrint(id), value, isLink);
  });

  layout->addWidget(dbb);
  setLayout(layout);

  auto const ppTitle = std::string("Place Page") + (info.IsBookmark() ? " (bookmarked)" : "");
  setWindowTitle(ppTitle.c_str());
}
