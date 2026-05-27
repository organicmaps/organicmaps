#pragma once

#include <QtWidgets/QWidget>

namespace place_page
{
class Info;
}

namespace qt
{
class DrawWidget;
}

class PlacePageDialogUser : public QWidget
{
  Q_OBJECT

public:
  PlacePageDialogUser(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info);
};
