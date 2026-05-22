#include "qt/place_page_dialog_user.hpp"
#include "qt/place_page_dialog_common.hpp"

#include "qt/qt_common/text_dialog.hpp"
#include "qt/qt_common/translations.hpp"

#include "indexer/validate_and_format_contacts.hpp"
#include "map/place_page_info.hpp"

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <string>

namespace
{
static int constexpr kMaxLengthOfPlacePageDescription = 500;

std::string getShortDescription(std::string const & description)
{
  std::string_view view(description);

  auto const paragraphStart = view.find("<p>");
  auto const paragraphEnd = view.find("</p>");

  if (paragraphStart == 0 && paragraphEnd != std::string::npos)
    view = view.substr(3, paragraphEnd - 3);

  if (view.length() > kMaxLengthOfPlacePageDescription)
    return std::string(view.substr(0, kMaxLengthOfPlacePageDescription - 3)) + "...";

  return std::string(view);
}

std::string_view stripSchemeFromURI(std::string_view uri)
{
  for (std::string_view prefix : {"https://", "http://"})
    if (uri.starts_with(prefix))
      return uri.substr(prefix.size());
  return uri;
}
}  // namespace

class QHLine : public QFrame
{
public:
  QHLine(QWidget * parent = nullptr) : QFrame(parent)
  {
    setFrameShape(QFrame::HLine);
    setFrameShadow(QFrame::Sunken);
  }
};

PlacePageDialogUser::PlacePageDialogUser(QWidget * parent, place_page::Info const & info) : QDialog(parent)
{
  auto const & title = info.GetTitle();

  QVBoxLayout * layout = new QVBoxLayout();
  {
    QVBoxLayout * header = new QVBoxLayout();

    if (!title.empty())
    {
      QLabel * titleLabel = new QLabel(QString::fromStdString("<h1>" + title + "</h1>"));
      titleLabel->setWordWrap(true);
      header->addWidget(titleLabel);
    }

    if (auto const subTitle = info.GetSubtitle(); !subTitle.empty())
    {
      QLabel * subtitleLabel = new QLabel(QString::fromStdString(subTitle));
      subtitleLabel->setWordWrap(true);
      header->addWidget(subtitleLabel);
    }

    if (auto const & address = info.GetSecondarySubtitle(); !address.empty())
    {
      QLabel * addressLabel = new QLabel(QString::fromStdString(address));
      addressLabel->setWordWrap(true);
      header->addWidget(addressLabel);
    }

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
      label->setWordWrap(true);
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
      addEntry("Bookmark", qt::Tr("yes").toStdString());

    // Wikipedia fragment
    if (auto const & wikipedia = info.GetMetadata(feature::Metadata::EType::FMD_WIKIPEDIA); !wikipedia.empty())
    {
      QString const wikipediaLabel = qt::Tr("read_in_wikipedia");
      QLabel * name = new QLabel(wikipediaLabel);
      name->setOpenExternalLinks(true);
      name->setTextInteractionFlags(Qt::TextBrowserInteraction);
      name->setText(QString::fromStdString("<a href=\"" + feature::Metadata::ToWikiURL(std::string(wikipedia)) + "\">" +
                                           wikipediaLabel.toStdString() + "</a>"));
      data->addWidget(name, row++, 0);
    }

    // Description
    if (auto const & description = info.GetWikiDescription(); !description.empty())
    {
      auto descriptionShort = getShortDescription(description);

      QLabel * value = new QLabel(QString::fromStdString(descriptionShort));
      value->setWordWrap(true);

      data->addWidget(value, row++, 0, 1, 2);

      QPushButton * wikiButton = new QPushButton(qt::Tr("placepage_more_button") + "…", value);
      wikiButton->setAutoDefault(false);
      connect(wikiButton, &QAbstractButton::clicked, this, [this, description, title]()
      {
        auto textDialog =
            TextDialog(this, QString::fromStdString(description), QString::fromStdString("Wikipedia: " + title));
        textDialog.exec();
      });

      data->addWidget(wikiButton, row++, 0, 1, 2, Qt::AlignLeft);
    }

    /// @todo Use a combo box like the developer dialog once the place page becomes
    /// a non-modal floating/dockable window.
    // Route refs
    if (auto routes = info.FormatRouteRefs(); !routes.empty())
      addEntry(qt::Tr("desktop_place_routes").toStdString(), routes);

    // Opening hours fragment
    if (auto openingHours = info.GetOpeningHours(); !openingHours.empty())
      addEntry(qt::Tr("desktop_place_opening_hours").toStdString(), std::string(openingHours));

    // Cuisine fragment
    if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
      addEntry(qt::Tr("cuisine").toStdString(), cuisines);

    // Entrance fragment
    // TODO

    // Phone fragment
    if (auto phoneNumber = info.GetMetadata(feature::Metadata::EType::FMD_PHONE_NUMBER); !phoneNumber.empty())
    {
      data->addWidget(new QLabel(qt::Tr("phone")), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='tel:" + std::string(phoneNumber) + "'>" +
                                                         std::string(phoneNumber) + "</a>"));
      value->setOpenExternalLinks(true);

      data->addWidget(value, row++, 1);
    }

    // Operator fragment
    if (auto operatorName = info.GetMetadata(feature::Metadata::EType::FMD_OPERATOR); !operatorName.empty())
      addEntry(qt::Tr("editor_operator").toStdString(), std::string(operatorName));

    // Wifi fragment
    if (info.HasWifi())
      addEntry(qt::Tr("category_wifi").toStdString(), qt::Tr("yes").toStdString());

    // Links fragment
    if (auto website = info.GetMetadata(feature::Metadata::EType::FMD_WEBSITE); !website.empty())
      addEntry(qt::Tr("website").toStdString(), std::string(stripSchemeFromURI(website)), true);

    if (auto email = info.GetMetadata(feature::Metadata::EType::FMD_EMAIL); !email.empty())
    {
      data->addWidget(new QLabel(qt::Tr("email")), row, 0);

      QLabel * value = new QLabel(
          QString::fromStdString("<a href='mailto:" + std::string(email) + "'>" + std::string(email) + "</a>"));
      value->setOpenExternalLinks(true);

      data->addWidget(value, row++, 1);
    }

    // Social networks
    {
      auto addSocialNetworkWidget = [data, &info, &row](std::string const label, feature::Metadata::EType const eType)
      {
        if (auto item = info.GetMetadata(eType); !item.empty())
        {
          data->addWidget(new QLabel(QString::fromStdString(label)), row, 0);

          QLabel * value = new QLabel(QString::fromStdString(
              "<a href='" + osm::socialContactToURL(eType, std::string(item)) + "'>" + std::string(item) + "</a>"));
          value->setOpenExternalLinks(true);
          value->setTextInteractionFlags(Qt::TextBrowserInteraction);

          data->addWidget(value, row++, 1);
        }
      };

      addSocialNetworkWidget("Facebook", feature::Metadata::EType::FMD_CONTACT_FACEBOOK);
      addSocialNetworkWidget("Instagram", feature::Metadata::EType::FMD_CONTACT_INSTAGRAM);
      addSocialNetworkWidget("Twitter", feature::Metadata::EType::FMD_CONTACT_TWITTER);
      addSocialNetworkWidget("VK", feature::Metadata::EType::FMD_CONTACT_VK);
      addSocialNetworkWidget("Line", feature::Metadata::EType::FMD_CONTACT_LINE);
    }

    if (auto wikimedia_commons = info.GetMetadata(feature::Metadata::EType::FMD_WIKIMEDIA_COMMONS);
        !wikimedia_commons.empty())
    {
      QString const commonsLabel = qt::Tr("wikimedia_commons");
      data->addWidget(new QLabel(commonsLabel), row, 0);

      QLabel * value = new QLabel(QString::fromStdString(
          "<a href='" + feature::Metadata::ToWikimediaCommonsURL(std::string(wikimedia_commons)) + "'>" +
          commonsLabel.toStdString() + "</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    // Level fragment
    if (auto level = info.GetMetadata(feature::Metadata::EType::FMD_LEVEL); !level.empty())
      addEntry(qt::Tr("level").toStdString(), std::string(level));

    // ATM fragment
    if (info.HasAtm())
      addEntry(qt::Tr("category_atm").toStdString(), qt::Tr("yes").toStdString());

    // Latlon fragment

    {
      ms::LatLon const ll = info.GetLatLon();
      addEntry(qt::Tr("desktop_place_coordinates").toStdString(),
               strings::to_string_dac(ll.m_lat, 7) + ", " + strings::to_string_dac(ll.m_lon, 7));
    }

    data->setColumnStretch(0, 0);
    data->setColumnStretch(1, 1);

    layout->addLayout(data);
  }

  layout->addStretch();

  {
    QHLine * line = new QHLine();
    layout->addWidget(line);
  }

  {
    QDialogButtonBox * dbb = new QDialogButtonBox();
    place_page_dialog::addCommonButtons(this, dbb, info.ShouldShowEditPlace());
    layout->addWidget(dbb, Qt::AlignCenter);
  }

  setLayout(layout);

  setWindowTitle(info.IsBookmark() ? qt::Tr("desktop_place_page_bookmarked") : qt::Tr("desktop_place_page"));
}
