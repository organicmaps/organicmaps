#include "qt/place_panel_common.hpp"

#include <QtWidgets/QPushButton>

namespace
{
const int kMinWidthOfShortDescription = 390;
}

void PlacePanel::addCommonButtons(QDialogButtonBox * dbb, bool shouldShowEditPlace)
{
  dbb->setCenterButtons(true);

  QPushButton * fromButton = new QPushButton("From");
  fromButton->setIcon(QIcon(":/navig64/point-start.png"));
  fromButton->setAutoDefault(false);
  connect(fromButton, &QAbstractButton::clicked, this, [this](){ routeFrom(*infoPtr); });
  dbb->addButton(fromButton, QDialogButtonBox::ActionRole);

  QPushButton * addStopButton = new QPushButton("Stop");
  addStopButton->setIcon(QIcon(":/navig64/point-intermediate.png"));
  addStopButton->setAutoDefault(false);
  connect(addStopButton, &QAbstractButton::clicked, this, [this](){ addStop(*infoPtr); });
  dbb->addButton(addStopButton, QDialogButtonBox::ActionRole);

  QPushButton * routeToButton = new QPushButton("To");
  routeToButton->setIcon(QIcon(":/navig64/point-finish.png"));
  routeToButton->setAutoDefault(false);
  connect(routeToButton, &QAbstractButton::clicked, this, [this](){ routeTo(*infoPtr); });
  dbb->addButton(routeToButton, QDialogButtonBox::ActionRole);

  if (shouldShowEditPlace)
  {
    QPushButton * editButton = new QPushButton("Edit Place");
    connect(editButton, &QAbstractButton::clicked, this, [this](){ editPlace(*infoPtr); });
    dbb->addButton(editButton, QDialogButtonBox::AcceptRole);
  }
}

PlacePanel::PlacePanel(QWidget * parent)
  : QWidget(parent)
{
  setFixedWidth(kMinWidthOfShortDescription);
}

void PlacePanel::setPlace(
  place_page::Info const & info,
  search::ReverseGeocoder::Address const & address
)
{
  infoPtr = &info;
}
