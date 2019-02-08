#pragma once

#include "search/reverse_geocoder.hpp"

#include <QtWidgets/QDialog>

namespace place_page
{
class Info;
}

class PlacePageDialog : public QDialog
{
  Q_OBJECT
public:
  PlacePageDialog(QWidget * parent, place_page::Info const & info,
                  search::ReverseGeocoder::Address const & address);

private slots:
  void OnClose();
  void OnEdit();
};
