#import "DeepLinkSearchData.h"
#import <CoreApi/Framework.h>
#include "drape_frontend/visual_params.hpp"
#include "geometry/mercator.hpp"
#include "geometry/latlon.hpp"

@implementation DeepLinkSearchData
- (instancetype)init:(DeeplinkUrlType)urlType success:(BOOL)success {
  self = [super init];
  if (self) {
    _urlType = urlType;
    _success = success;
    auto const &request = GetFramework().GetParsedSearchRequest();
    ms::LatLon const center = GetFramework().GetParsedCenterLatLon();
    _query = [@((request.m_query + " ").c_str()) stringByRemovingPercentEncoding];
    _locale = @(request.m_locale.c_str());
    _centerLat = center.m_lat;
    _centerLon = center.m_lon;
    _isSearchOnMap = request.m_isSearchOnMap;
  }
  return self;
}

- (void)onViewportChanged:(int)zoomLevel {
  auto const center = mercator::FromLatLon(_centerLat, _centerLon);
  auto const rect = df::GetRectForDrawScale(zoomLevel, center);
  GetFramework().GetSearchAPI().OnViewportChanged(rect);
}
@end
