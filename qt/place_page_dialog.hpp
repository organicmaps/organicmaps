#pragma once

#include "map/routing_mark.hpp"
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

  std::optional<RouteMarkType> GetRoutePointAddMode() const;
  void SetRoutePointAddMode(RouteMarkType);

private slots:
  void OnClose();
  void OnEdit();

private:
  std::optional<RouteMarkType> m_routePointAddMode;
};
