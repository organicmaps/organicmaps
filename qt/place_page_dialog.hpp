#pragma once

#include <QtWidgets/QDialog>

namespace place_page
{
class Info;
}
namespace search
{
struct AddressInfo;
}

class PlacePageDialog : public QDialog
{
  Q_OBJECT
public:
  PlacePageDialog(QWidget * parent, place_page::Info const & info,
                  search::AddressInfo const & address);

private slots:
  void OnClose();
  void OnEdit();
};
