#include "qt/place_page_dialog.hpp"

#include "map/place_page_info.hpp"

#include <string>

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

using namespace std;

string GenerateStars(int count)
{
  string stars;
  for (int i = 0; i < count; ++i)
    stars.append("★");
  return stars;
}

PlacePageDialog::PlacePageDialog(QWidget * parent, place_page::Info const & info,
                                 search::ReverseGeocoder::Address const & address)
  : QDialog(parent)
{
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
  };

  {
    ms::LatLon const ll = info.GetLatLon();
    addEntry("lat, lon", strings::to_string_dac(ll.m_lat, 7) + ", " + strings::to_string_dac(ll.m_lon, 7));
  }

  addEntry("CountryId", info.GetCountryId());

  if (auto const & title = info.GetTitle(); !title.empty())
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

  using PropID = osm::MapObject::MetadataID;

  if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
    addEntry(DebugPrint(PropID::FMD_CUISINE), cuisines);

  if (auto const & descr = info.GetDescription(); !descr.empty())
    addEntry("Description size", std::to_string(descr.size()));

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

  QDialogButtonBox * dbb = new QDialogButtonBox();
  QPushButton * closeButton = new QPushButton("Close");
  closeButton->setDefault(true);
  connect(closeButton, &QAbstractButton::clicked, this, &PlacePageDialog::OnClose);
  dbb->addButton(closeButton, QDialogButtonBox::RejectRole);

  if (info.ShouldShowEditPlace())
  {
    QPushButton * editButton = new QPushButton("Edit Place");
    connect(editButton, &QAbstractButton::clicked, this, &PlacePageDialog::OnEdit);
    dbb->addButton(editButton, QDialogButtonBox::AcceptRole);
  }
  grid->addWidget(dbb);
  setLayout(grid);

  string const title = string("Place Page") + (info.IsBookmark() ? " (bookmarked)" : "");
  setWindowTitle(title.c_str());
}

void PlacePageDialog::OnClose() { reject(); }
void PlacePageDialog::OnEdit() { accept(); }
