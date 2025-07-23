#pragma once

#include "search/reverse_geocoder.hpp"

#include <QtWidgets/QDialog>

namespace place_page
{
class Info;
}

class PlacePageDialogUser : public QDialog
{
  Q_OBJECT

public:
  PlacePageDialogUser(QWidget * parent, place_page::Info const & info,
                      search::ReverseGeocoder::Address const & address);
};
