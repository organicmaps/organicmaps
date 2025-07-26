#import "DeepLinkSearchData.h"
#import <CoreApi/Framework.h>
#include "drape_frontend/visual_params.hpp"
#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

@implementation DeepLinkSearchData
- (instancetype)init
{
  self = [super init];
  if (self)
  {
    auto const & request = GetFramework().GetParsedSearchRequest();
    ms::LatLon const center = GetFramework().GetParsedCenterLatLon();
    _query = [@((request.m_query + " ").c_str()) stringByRemovingPercentEncoding];
    _locale = @(request.m_locale.c_str());
    _centerLat = center.m_lat;
    _centerLon = center.m_lon;
    _isSearchOnMap = request.m_isSearchOnMap;
  }
  return self;
}

- (BOOL)hasValidCenterLatLon
{
  return _centerLat != ms::LatLon::kInvalid && _centerLon != ms::LatLon::kInvalid;
}

- (void)onViewportChanged:(int)zoomLevel
{
  auto const center = mercator::FromLatLon(_centerLat, _centerLon);
  auto const rect = df::GetRectForDrawScale(zoomLevel, center);
  GetFramework().GetSearchAPI().OnViewportChanged(rect);
}
@end
