#include "partners_api/yango_api.hpp"

namespace taxi::yango
{
RideRequestLinks Api::GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                          ms::LatLon const & to) const
{
  std::ostringstream link;

  link << "https://2187871.redirect.appmetrica.yandex.com/route?start-lat=" << from.m_lat
       << "&start-lon=" << from.m_lon << "&end-lat=" << to.m_lat << "&end-lon=" << to.m_lon
       << "&utm_source=mapsme&utm_medium=none&ref=8d20bf0f9e4749c48822358cdaf6a6c7"
          "&appmetrica_tracking_id=1179283486394045767&level=50";

  return {link.str(), link.str()};;
}
}  // namespace taxi::yango
