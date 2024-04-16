#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>

namespace place_page_dialog
{
enum PressedButton : int {
  RouteFrom,
  AddStop,
  RouteTo,
  Close,
  EditPlace
};

void addCommonButtons(QDialog * this_, QDialogButtonBox * dbb, bool shouldShowEditPlace);
}
