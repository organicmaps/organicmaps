#include "qt/place_page_dialog.hpp"

#include "qt/qt_common/text_dialog.hpp"

#include "map/place_page_info.hpp"
#include "indexer/validate_and_format_contacts.hpp"
#include "platform/settings.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include <qboxlayout.h>
#include <qdialog.h>
#include <qgridlayout.h>
#include <qnamespace.h>
#include <sstream>
#include <string>

const int place_page_description_max_length = 500;
const int short_description_minimum_width = 390;

namespace {
  std::string stripSchemeFromURI(std::string const & input) {
    std::string result = input;
    if (size(input) > 8 && ("https://" == input.substr(0, 8))) {
      result = result.substr(8, size(input));
    } else if (size(input) > 7 && ("http://" == input.substr(0, 7))) {
      result = result.substr(7, size(input));
    }
    return result;
  }
}

class QHLine : public QFrame
{
public:
  QHLine(QWidget * parent = nullptr) : QFrame(parent)
  {
    setFrameShape(QFrame::HLine);
    setFrameShadow(QFrame::Sunken);
  }
};

std::string getShortDescription(std::string description)
{
  size_t paragraphStart = description.find("<p>");
  size_t paragraphEnd = description.find("</p>");
  if (paragraphStart == 0 && paragraphEnd != std::string::npos)
    description = description.substr(3, paragraphEnd);

  if (description.length() > place_page_description_max_length)
  {
    description = description.substr(0, place_page_description_max_length-3) + "...";
  }

  return description;
}

PlacePageDialog::PlacePageDialog(QWidget * parent, place_page::Info const & info,
                                 search::ReverseGeocoder::Address const & address)
  : QDialog(parent)
{
  using PropID = osm::MapObject::MetadataID;

  auto const & title = info.GetTitle();

  QVBoxLayout * layout = new QVBoxLayout();
  {
    QVBoxLayout * header = new QVBoxLayout();

    if (!title.empty())
      header->addWidget(new QLabel(QString::fromStdString("<h1>" + title + "</h1>")));

    if (auto subTitle = info.GetSubtitle(); !subTitle.empty())
      header->addWidget(new QLabel(QString::fromStdString(subTitle)));

    if (auto addressFormatted = address.FormatAddress(); !addressFormatted.empty())
      header->addWidget(new QLabel(QString::fromStdString(addressFormatted)));

    layout->addLayout(header);
  }

  {
    QHLine * line = new QHLine();
    layout->addWidget(line);
  }

  {
    QGridLayout * data = new QGridLayout();

    int row = 0;

    auto const addEntry = [data, &row](std::string const & key, std::string const & value, bool isLink = false)
    {
      data->addWidget(new QLabel(QString::fromStdString(key)), row, 0);
      QLabel * label = new QLabel(QString::fromStdString(value));
      label->setTextInteractionFlags(Qt::TextSelectableByMouse);
      if (isLink)
      {
        label->setOpenExternalLinks(true);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setText(QString::fromStdString("<a href=\"" + value + "\">" + value + "</a>"));
      }
      data->addWidget(label, row++, 1);
      return label;
    };

    if (info.IsBookmark())
    {
      addEntry("Bookmark", "Yes");
    }


    // Wikipedia fragment
    if (auto const & wikipedia = info.GetMetadata(feature::Metadata::EType::FMD_WIKIPEDIA); !wikipedia.empty()){
      QLabel * name = new QLabel("Wikipedia");
      name->setOpenExternalLinks(true);
      name->setTextInteractionFlags(Qt::TextBrowserInteraction);
      name->setText(QString::fromStdString("<a href=\"" + feature::Metadata::ToWikiURL(std::string(wikipedia)) + "\">Wikipedia</a>"));
      data->addWidget(name, row++, 0);
    }

    // Description
    if (auto description = info.GetWikiDescription(); !description.empty())
    {
      auto descriptionShort = getShortDescription(description);

      QLabel * value = new QLabel(QString::fromStdString(descriptionShort));
      value->setWordWrap(true);
      value->setMinimumWidth(short_description_minimum_width);

      data->addWidget(value, row++, 0, 1, 2);

      QPushButton * wikiButton = new QPushButton("More...", value);
      wikiButton->setAutoDefault(false);
      connect(wikiButton, &QAbstractButton::clicked, this, [this, description, title]()
      {
        auto textDialog = TextDialog(this, QString::fromStdString(description), QString::fromStdString("Wikipedia: " + title));
        textDialog.exec();
      });

      data->addWidget(wikiButton, row++, 0, 1, 2, Qt::AlignLeft);
    }

    // Opening hours fragment
    if (auto openingHours = info.GetOpeningHours(); !openingHours.empty())
      addEntry("Opening hours", std::string(openingHours));

    // Cuisine fragment
    if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
      addEntry("Cuisine", cuisines);

    // Entrance fragment
    // TODO

    // Phone fragment
    if (auto phoneNumber = info.GetMetadata(feature::Metadata::EType::FMD_PHONE_NUMBER); !phoneNumber.empty()){
      data->addWidget(new QLabel("Phone"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='tel:" + std::string(phoneNumber) + "'>" + std::string(phoneNumber) + "</a>"));
      value->setOpenExternalLinks(true);

      data->addWidget(value, row++, 1);
    }

    // Operator fragment
    if (auto operatorName = info.GetMetadata(feature::Metadata::EType::FMD_OPERATOR); !operatorName.empty())
      addEntry("Operator", std::string(operatorName));

    // Wifi fragment
    if (info.HasWifi()){
      addEntry("Wi-Fi", "Yes");
    }

    // Links fragment
    if (auto website = info.GetMetadata(feature::Metadata::EType::FMD_WEBSITE); !website.empty())
      addEntry("Website", stripSchemeFromURI(std::string(website)), true);

    if (auto email = info.GetMetadata(feature::Metadata::EType::FMD_EMAIL); !email.empty()){
      data->addWidget(new QLabel("Email"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='mailto:" + std::string(email) + "'>" + std::string(email) + "</a>"));
      value->setOpenExternalLinks(true);

      data->addWidget(value, row++, 1);
    }

    if (auto facebook = info.GetMetadata(feature::Metadata::EType::FMD_CONTACT_FACEBOOK); !facebook.empty()){
      data->addWidget(new QLabel("Facebook"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='" + osm::socialContactToURL(feature::Metadata::EType::FMD_CONTACT_FACEBOOK, std::string(facebook)) + "'>" + std::string(facebook) + "</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    if (auto instagram = info.GetMetadata(feature::Metadata::EType::FMD_CONTACT_INSTAGRAM); !instagram.empty()){
      data->addWidget(new QLabel("Instagram"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='" + osm::socialContactToURL(feature::Metadata::EType::FMD_CONTACT_INSTAGRAM, std::string(instagram)) + "'>" + std::string(instagram) + "</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    if (auto twitter = info.GetMetadata(feature::Metadata::EType::FMD_CONTACT_TWITTER); !twitter.empty()){
      data->addWidget(new QLabel("Twitter"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='" + osm::socialContactToURL(feature::Metadata::EType::FMD_CONTACT_TWITTER, std::string(twitter)) + "'>" + std::string(twitter) + "</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    if (auto vk = info.GetMetadata(feature::Metadata::EType::FMD_CONTACT_VK); !vk.empty()){
      data->addWidget(new QLabel("VK"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='" + osm::socialContactToURL(feature::Metadata::EType::FMD_CONTACT_VK, std::string(vk)) + "'>" + std::string(vk) + "</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    if (auto line = info.GetMetadata(feature::Metadata::EType::FMD_CONTACT_LINE); !line.empty()){
      data->addWidget(new QLabel("Line"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='" + osm::socialContactToURL(feature::Metadata::EType::FMD_CONTACT_LINE, std::string(line)) + "'>" + std::string(line) + "</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    if (auto wikimedia_commons = info.GetMetadata(feature::Metadata::EType::FMD_WIKIMEDIA_COMMONS); !wikimedia_commons.empty()){
      QLabel * value = new QLabel(QString::fromStdString("<a href='" + feature::Metadata::ToWikimediaCommonsURL(std::string(wikimedia_commons)) + "'>Wikimedia Commons</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 0);
    }

    // Level fragment
    if (auto level = info.GetMetadata(feature::Metadata::EType::FMD_LEVEL); !level.empty()){
      addEntry("Level", std::string(level));
    }

    // ATM fragment
    if (info.HasAtm()){
      addEntry("ATM", "Yes");
    }

    // Latlon fragment

    {
      ms::LatLon const ll = info.GetLatLon();
      addEntry("Coordinates", strings::to_string_dac(ll.m_lat, 7) + ", " + strings::to_string_dac(ll.m_lon, 7));
    }

    layout->addLayout(data);
  }

  // Advanced
  bool developerMode;
  if (settings::Get(settings::kDeveloperMode, developerMode) && developerMode)
  {
    {
      QHLine * line = new QHLine();
      layout->addWidget(line);
    }

    layout->addWidget(new QLabel("<b>Advanced</b>"));

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
        label->setText(QString::fromStdString("<a href=\"" + value + "\">" + stripSchemeFromURI(value) + "</a>"));
      }
      grid->addWidget(label, row++, 1);
      return label;
    };

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

    addEntry("CountryId", info.GetCountryId());

    if (info.IsFeature())
    {
      addEntry("Feature ID", DebugPrint(info.GetID()));
      addEntry("Raw Types", DebugPrint(info.GetTypes()));
    }

    auto const layer = info.GetLayer();
    if (layer != feature::LAYER_EMPTY)
      addEntry("Layer", std::to_string(layer));

    if(info.HasMetadataReadable()){
      grid->addWidget(new QLabel("<b>Metadata</b>"), row++, 0, 1, 2);

      info.ForEachMetadataReadable([&addEntry](PropID id, std::string const & value)
      {
        switch (id)
        { // SKIP those that are already listed in the non-advanced (non-developer) mode
        case PropID::FMD_EMAIL:
        case PropID::FMD_CONTACT_FACEBOOK:
        case PropID::FMD_CONTACT_INSTAGRAM:
        case PropID::FMD_CONTACT_TWITTER:
        case PropID::FMD_CONTACT_VK:
        case PropID::FMD_CONTACT_LINE:
        case PropID::FMD_LEVEL:
        case PropID::FMD_OPERATOR:
        case PropID::FMD_PHONE_NUMBER:
        case PropID::FMD_WEBSITE:
        case PropID::FMD_WIKIPEDIA:
        case PropID::FMD_WIKIMEDIA_COMMONS:
          break;
        default:
          addEntry(DebugPrint(id), value, false);
          break;
        }

      });
    }

    layout->addLayout(grid);
  }

  {
    QHLine * line = new QHLine();
    layout->addWidget(line);
  }

  {
    QDialogButtonBox * dbb = new QDialogButtonBox();

    QPushButton * closeButton = new QPushButton("Close");
    closeButton->setDefault(true);
    connect(closeButton, &QAbstractButton::clicked, this, &PlacePageDialog::OnClose);
    dbb->addButton(closeButton, QDialogButtonBox::RejectRole);

    if (info.ShouldShowEditPlace())
    {
      QPushButton * editButton = new QPushButton("Edit Place");
      connect(editButton, &QAbstractButton::clicked, this, &PlacePageDialog::OnEdit);
      dbb->addButton(editButton, QDialogButtonBox::ActionRole);
    }

    layout->addWidget(dbb, Qt::AlignCenter);
  }

  setLayout(layout);

  auto const ppTitle = std::string("Place Page") + (info.IsBookmark() ? " (bookmarked)" : "");
  setWindowTitle(ppTitle.c_str());
}

void PlacePageDialog::OnClose() { reject(); }
void PlacePageDialog::OnEdit() { accept(); }
