#include "qt/place_page_dialog_common.hpp"
#include "qt/qt_common/translations.hpp"

#include <QtWidgets/QPushButton>

namespace place_page_dialog
{
void addCommonButtons(QDialog * this_, QDialogButtonBox * dbb, bool shouldShowEditPlace)
{
  dbb->setCenterButtons(true);

  QPushButton * fromButton = new QPushButton(qt::Tr("p2p_from_here"));
  fromButton->setIcon(QIcon(":/navig64/point-start.png"));
  fromButton->setAutoDefault(false);
  this_->connect(fromButton, &QAbstractButton::clicked, this_, [this_] { this_->done(RouteFrom); });
  dbb->addButton(fromButton, QDialogButtonBox::ActionRole);

  QPushButton * addStopButton = new QPushButton(qt::Tr("placepage_add_stop"));
  addStopButton->setIcon(QIcon(":/navig64/point-intermediate.png"));
  addStopButton->setAutoDefault(false);
  this_->connect(addStopButton, &QAbstractButton::clicked, this_, [this_] { this_->done(AddStop); });
  dbb->addButton(addStopButton, QDialogButtonBox::ActionRole);

  QPushButton * routeToButton = new QPushButton(qt::Tr("p2p_to_here"));
  routeToButton->setIcon(QIcon(":/navig64/point-finish.png"));
  routeToButton->setAutoDefault(false);
  this_->connect(routeToButton, &QAbstractButton::clicked, this_, [this_] { this_->done(RouteTo); });
  dbb->addButton(routeToButton, QDialogButtonBox::ActionRole);

  QPushButton * closeButton = new QPushButton(qt::Tr("close"));
  closeButton->setDefault(true);
  this_->connect(closeButton, &QAbstractButton::clicked, this_, [this_] { this_->done(place_page_dialog::Close); });
  dbb->addButton(closeButton, QDialogButtonBox::RejectRole);

  if (shouldShowEditPlace)
  {
    QPushButton * editButton = new QPushButton(qt::Tr("edit_place"));
    this_->connect(editButton, &QAbstractButton::clicked, this_,
                   [this_] { this_->done(place_page_dialog::EditPlace); });
    dbb->addButton(editButton, QDialogButtonBox::AcceptRole);
  }
}
}  // namespace place_page_dialog
