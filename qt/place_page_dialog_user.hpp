#pragma once

#include "qt/place_page_dialog_common.hpp"

namespace place_page
{
class Info;
}

namespace qt
{
class DrawWidget;
}

namespace place_page_dialog_user
{
std::string getShortDescription(std::string const & description);
}
class PlacePageDialogUser : public PlacePageDialogCommon
{
  Q_OBJECT

public:
  PlacePageDialogUser(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info, const QList<QAction *> &actions);
};
