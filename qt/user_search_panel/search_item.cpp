#include "qt/user_search_panel/search_item.hpp"

#include "search/result.hpp"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

namespace qt
{

SearchItem::SearchItem(search::Result const & result, QWidget * parent) : QWidget(parent)
{
  QGridLayout * layout = new QGridLayout;
  layout->setVerticalSpacing(0);

  QLabel * nameLabel = CreateLabelWithSearchHighlight(result);
  nameLabel->setWordWrap(true);

  switch (result.GetResultType())
  {
  case search::Result::Type::SuggestFromFeature:
  case search::Result::Type::PureSuggest:
  {
    QLabel * iconLabel = new QLabel;
    // TODO (@zagto): Use pixelToDeviceRatio
    iconLabel->setPixmap(QIcon(":/navig64/search.png").pixmap(24));
    layout->addWidget(iconLabel, 0, 0);
    layout->addWidget(nameLabel, 0, 1);
    layout->setColumnStretch(1, 1);
    break;
  }
  case search::Result::Type::Feature:
  case search::Result::Type::Postcode:
  {
    QFont nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    // TODO (@zagto): GetDescHighlightRange
    QLabel * featureDescriptionLabel = new QLabel;
    featureDescriptionLabel->setTextFormat(Qt::PlainText);
    featureDescriptionLabel->setText(QString::fromStdString(result.GetFeatureDescription()));
    featureDescriptionLabel->setWordWrap(true);

    // TODO (@zagto): implement
    QLabel * openLabel = new QLabel("Open");
    QLabel * distanceLabel = new QLabel("1,2 km");

    QLabel * addressLabel = new QLabel;
    addressLabel->setTextFormat(Qt::PlainText);
    addressLabel->setText(QString::fromStdString(result.GetAddress()));
    addressLabel->setWordWrap(true);

    layout->addWidget(nameLabel, 0, 0, 1, 3);
    layout->addWidget(featureDescriptionLabel, 1, 0, 1, 2);
    layout->addWidget(openLabel, 1, 1, 1, 2, Qt::AlignRight);
    layout->addWidget(addressLabel, 2, 0, 1, 2);
    layout->addWidget(distanceLabel, 2, 2, 1, 1, Qt::AlignRight);
    layout->setColumnStretch(0, 1);
    break;
  }
  case search::Result::Type::LatLon:
  {
    QLabel * iconLabel = new QLabel;
    // TODO (@zagto): Use pixelToDeviceRatio
    // TODO (@zagto): use the same icon we use for coordinates when viewing feature properties
    iconLabel->setPixmap(QIcon(":/navig64/location.png").pixmap(24));
    layout->addWidget(iconLabel, 0, 0);
    layout->addWidget(nameLabel, 0, 1);
    break;
  }
  }

  setLayout(layout);
}

QLabel * SearchItem::CreateLabelWithSearchHighlight(search::Result const & result)
{
  QString const name = QString::fromStdString(result.GetString());
  QString strHigh;
  int pos = 0;
  for (size_t r = 0; r < result.GetHighlightRangesCount(); ++r)
  {
    // TODO (@zagto): html-escape input
    std::pair<uint16_t, uint16_t> const & range = result.GetHighlightRange(r);
    strHigh.append(name.mid(pos, range.first - pos).toHtmlEscaped());
    // TODO (@zagto): don't hardcode color
    strHigh.append("<span style=\"background-color: #ABDCF6\">");
    strHigh.append(name.mid(range.first, range.second).toHtmlEscaped());
    strHigh.append("</span>");

    pos = range.first + range.second;
  }
  strHigh.append(name.mid(pos).toHtmlEscaped());

  return new QLabel(strHigh);
}

}  // namespace qt
