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

class PlacePageDialogDeveloper : public QWidget
{
  Q_OBJECT

public:
  PlacePageDialogDeveloper(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info);

private:
  qt::DrawWidget * m_drawWidget;
};
