#include "qt/place_page_dialog_user.hpp"

#include "qt/draw_widget.hpp"
#include "qt/qt_common/text_dialog.hpp"

#include "map/bookmark.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include "indexer/validate_and_format_contacts.hpp"

#include "kml/types.hpp"

#include "drape/color.hpp"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QFrame>
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

PlacePageDialogUser::PlacePageDialogUser(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info)
  : PlacePageDialogCommon(parent, drawWidget, info)
{
  using namespace place_page_dialog;
  auto const & title = info.GetTitle();
  QVBoxLayout * contentLayout = GetContentLayout();

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

    contentLayout->addLayout(header);
  }

  contentLayout->addWidget(new QHLine());

  {
    QGridLayout * data = new QGridLayout();

    int row = 0;

    if (info.IsBookmark())
    {
      addEntry(data, row, "Bookmark", "Yes");

      // Bookmark color: a swatch button that opens QColorDialog and applies an arbitrary color.
      auto const bookmarkId = info.GetBookmarkId();

      data->addWidget(new QLabel("Color"), row, 0);
      QPushButton * colorButton = new QPushButton();
      colorButton->setAutoDefault(false);

      auto const setSwatch = [colorButton](dp::Color const & c)
      {
        colorButton->setStyleSheet(
            QString("background-color: rgb(%1, %2, %3);").arg(c.GetRed()).arg(c.GetGreen()).arg(c.GetBlue()));
      };
      if (auto const * mark = drawWidget->GetFramework().GetBookmarkManager().GetBookmark(bookmarkId))
        setSwatch(mark->GetColorForRendering());

      connect(colorButton, &QAbstractButton::clicked, this, [this, bookmarkId, setSwatch]()
      {
        auto & bm = GetDrawWidget()->GetFramework().GetBookmarkManager();
        auto const * mark = bm.GetBookmark(bookmarkId);
        if (mark == nullptr)
          return;

        auto const current = mark->GetColorForRendering();
        QColor const picked = QColorDialog::getColor(QColor(current.GetRed(), current.GetGreen(), current.GetBlue()),
                                                     this, "Bookmark color");
        if (!picked.isValid())
          return;

        dp::Color const newColor(picked.red(), picked.green(), picked.blue());
        // Update the swatch and last-edited color first; the EditSession below notifies the engine
        // on destruction (the last action in this handler), so nothing here can run after it.
        setSwatch(newColor);
        bm.SetLastEditedBmColor(kml::MakeCustomBookmarkColorData(newColor));

        auto editSession = bm.GetEditSession();
        if (auto * editable = editSession.GetBookmarkForEdit(bookmarkId))
          editable->SetColor(newColor);
      });

      data->addWidget(colorButton, row++, 1);
    }

    // Wikipedia fragment
    if (auto const & wikipedia = info.GetMetadata(feature::Metadata::EType::FMD_WIKIPEDIA); !wikipedia.empty())
    {
      QLabel * name = new QLabel("Wikipedia");
      name->setOpenExternalLinks(true);
      name->setTextInteractionFlags(Qt::TextBrowserInteraction);
      name->setText(QString::fromStdString("<a href=\"" + feature::Metadata::ToWikiURL(std::string(wikipedia)) +
                                           "\">Wikipedia</a>"));
      data->addWidget(name, row++, 0);
    }

    // Description
    if (auto const & description = info.GetWikiDescription(); !description.empty())
    {
      auto descriptionShort = getShortDescription(description);

      QLabel * value = new QLabel(QString::fromStdString(descriptionShort));
      value->setWordWrap(true);

      data->addWidget(value, row++, 0, 1, 2);

      QPushButton * wikiButton = new QPushButton("More...", value);
      wikiButton->setAutoDefault(false);
      connect(wikiButton, &QAbstractButton::clicked, this, [this, description, title]()
      {
        auto textDialog =
            TextDialog(this, QString::fromStdString(description), QString::fromStdString("Wikipedia: " + title));
        textDialog.exec();
      });

      data->addWidget(wikiButton, row++, 0, 1, 2, Qt::AlignLeft);
    }

    addRoutesRow(data, row, drawWidget, info);

    // Opening hours fragment
    if (auto openingHours = info.GetOpeningHours(); !openingHours.empty())
      addEntry(data, row, "Opening hours", std::string(openingHours));

    // Cuisine fragment
    if (auto cuisines = info.FormatCuisines(); !cuisines.empty())
      addEntry(data, row, "Cuisine", cuisines);

    // Entrance fragment
    // TODO

    // Phone fragment
    if (auto phoneNumber = info.GetMetadata(feature::Metadata::EType::FMD_PHONE_NUMBER); !phoneNumber.empty())
    {
      data->addWidget(new QLabel("Phone"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString("<a href='tel:" + std::string(phoneNumber) + "'>" +
                                                         std::string(phoneNumber) + "</a>"));
      value->setOpenExternalLinks(true);

      data->addWidget(value, row++, 1);
    }

    // Operator fragment
    if (auto operatorName = info.GetMetadata(feature::Metadata::EType::FMD_OPERATOR); !operatorName.empty())
      addEntry(data, row, "Operator", std::string(operatorName));

    // Wifi fragment
    if (info.HasWifi())
      addEntry(data, row, "Wi-Fi", "Yes");

    // Links fragment
    if (auto website = info.GetMetadata(feature::Metadata::EType::FMD_WEBSITE); !website.empty())
      addEntry(data, row, "Website", std::string(stripSchemeFromURI(website)), true);

    if (auto email = info.GetMetadata(feature::Metadata::EType::FMD_EMAIL); !email.empty())
    {
      data->addWidget(new QLabel("Email"), row, 0);

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
      data->addWidget(new QLabel("Wikimedia Commons"), row, 0);

      QLabel * value = new QLabel(QString::fromStdString(
          "<a href='" + feature::Metadata::ToWikimediaCommonsURL(std::string(wikimedia_commons)) +
          "'>Wikimedia Commons</a>"));
      value->setOpenExternalLinks(true);
      value->setTextInteractionFlags(Qt::TextBrowserInteraction);

      data->addWidget(value, row++, 1);
    }

    // Level fragment
    if (auto level = info.GetMetadata(feature::Metadata::EType::FMD_LEVEL); !level.empty())
      addEntry(data, row, "Level", std::string(level));

    // ATM fragment
    if (info.HasAtm())
      addEntry(data, row, "ATM", "Yes");

    // Latlon fragment
    addCoordinatesRow(data, row, info);

    data->setColumnStretch(0, 0);
    data->setColumnStretch(1, 1);

    contentLayout->addLayout(data);
  }

  contentLayout->addStretch();
}
