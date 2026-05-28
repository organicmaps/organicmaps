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

class PlacePageDialogDeveloper : public PlacePageDialogCommon
{
  Q_OBJECT

public:
  PlacePageDialogDeveloper(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info);
};
