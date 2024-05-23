#pragma once

#include "qt/place_panel_common.hpp"

class PlacePanelUser : public PlacePanel
{
  Q_OBJECT
public:
  PlacePanelUser(QWidget * parent);

  virtual void setPlace(
    place_page::Info const & info,
    search::ReverseGeocoder::Address const & address
  );

signals:
  void showPlace();
  void hidePlace();
  void editPlace(place_page::Info const & info);
};
