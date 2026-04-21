#pragma once

#include <QtWidgets/QDialog>

class Framework;

namespace place_page
{
class Info;
}

class PlacePageDialogDeveloper : public QDialog
{
  Q_OBJECT

public:
  PlacePageDialogDeveloper(QWidget * parent, place_page::Info const & info, Framework & framework);

private:
  Framework & m_framework;
};
