#pragma once

#include <QtWidgets/QDialog>

namespace place_page
{
class Info;
}

class PlacePageDialogDeveloper : public QDialog
{
  Q_OBJECT

public:
  PlacePageDialogDeveloper(QWidget * parent, place_page::Info const & info);
};
