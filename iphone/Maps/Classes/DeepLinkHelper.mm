#import "DeepLinkHelper.h"

#import <Crashlytics/Crashlytics.h>

#include <CoreApi/Framework.h>
#import "Statistics.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "MWMRouter.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMCoreRouterType.h"
#import "MWMMapViewControlsManager.h"

#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

@implementation DeepLinkHelper

+ (void)handleGeoUrl:(NSURL *)url {
  NSLog(@"deeplink: handleGeoUrl %@", url);
  if (GetFramework().ShowMapForURL(url.absoluteString.UTF8String)) {
    [Statistics logEvent:kStatEventName(kStatApplication, kStatImport)
          withParameters:@{kStatValue : url.scheme}];
    [MapsAppDelegate.theApp showMap];
  }
}

+ (void)handleFileUrl:(NSURL *)url {
  NSLog(@"deeplink: handleFileUrl %@", url);
  GetFramework().AddBookmarksFile(url.relativePath.UTF8String, false /* isTemporaryFile */);
}

+ (void)handleCommonUrl:(NSURL *)url {
  NSLog(@"deeplink: handleCommonUrl %@", url);
  using namespace url_scheme;

  Framework &f = GetFramework();
  auto const parsingType = f.ParseAndSetApiURL(url.absoluteString.UTF8String);
  switch (parsingType) {
    case ParsedMapApi::ParsingResult::Incorrect:
      LOG(LWARNING, ("Incorrect parsing result for url:", url));
      break;
    case ParsedMapApi::ParsingResult::Route: {
      auto const parsedData = f.GetParsedRoutingData();
      auto const points = parsedData.m_points;
      if (points.size() == 2) {
        auto p1 = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.front()
                                                                type:MWMRoutePointTypeStart
                                                   intermediateIndex:0];
        auto p2 = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.back()
                                                                type:MWMRoutePointTypeFinish
                                                   intermediateIndex:0];
        [MWMRouter buildApiRouteWithType:routerType(parsedData.m_type)
                              startPoint:p1
                             finishPoint:p2];
      } else {
        NSError *err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                                  code:5
                                              userInfo:@{ @"Description" : @"Invalid number of route points",
                                                          @"URL" : url }];
        [[Crashlytics sharedInstance] recordError:err];
      }

      [MapsAppDelegate.theApp showMap];
      break;
    }
    case ParsedMapApi::ParsingResult::Map:
      if (f.ShowMapForURL(url.absoluteString.UTF8String))
        [MapsAppDelegate.theApp showMap];
      break;
    case ParsedMapApi::ParsingResult::Search: {
      int constexpr kSearchInViewportZoom = 16;
      
      auto const &request = f.GetParsedSearchRequest();

      auto query = [@((request.m_query + " ").c_str()) stringByRemovingPercentEncoding];
      auto locale = @(request.m_locale.c_str());

      // Set viewport only when cll parameter was provided in url.
      if (request.m_centerLat != 0.0 || request.m_centerLon != 0.0) {
        [MapViewController setViewport:request.m_centerLat
                                   lon:request.m_centerLon
                             zoomLevel:kSearchInViewportZoom];
        
        // We need to update viewport for search api manually because of drape engine
        // will not notify subscribers when search view is shown.
        if (!request.m_isSearchOnMap)
        {
          auto const center = mercator::FromLatLon(request.m_centerLat, request.m_centerLon);
          auto const rect = df::GetRectForDrawScale(kSearchInViewportZoom, center);
          f.GetSearchAPI().OnViewportChanged(rect);
        }
      }

      if (request.m_isSearchOnMap) {
        [MWMMapViewControlsManager.manager searchTextOnMap:query forInputLocale:locale];
      } else {
        [MWMMapViewControlsManager.manager searchText:query forInputLocale:locale];
      }

      break;
    }
    case ParsedMapApi::ParsingResult::Catalogue:
      [MapViewController.sharedController openCatalogDeeplink:url animated:NO utm:MWMUTMNone];
      break;
    case ParsedMapApi::ParsingResult::CataloguePath:
      [MapViewController.sharedController openCatalogDeeplink:url animated:NO utm:MWMUTMNone];
      break;
    case ParsedMapApi::ParsingResult::Lead: break;
  }
}

@end
