#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>

namespace place_page_dialog
{
enum PressedButton : int
{
  Close = QDialog::Rejected,
  RouteFrom,
  AddStop,
  RouteTo,
  EditPlace
};

void addCommonButtons(QDialog * this_, QDialogButtonBox * dbb, bool shouldShowEditPlace);
}  // namespace place_page_dialog
