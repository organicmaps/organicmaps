#import "MWMFrameworkUtils.h"
#import "MWMWatchLocationTracker.h"
#include "Framework.h"

#include "indexer/scales.hpp"
#include "indexer/mercator.hpp"

#include "platform/location.hpp"

extern NSString * const kSearchResultTitleKey;
extern NSString * const kSearchResultCategoryKey;
extern NSString * const kSearchResultPointKey;

@implementation MWMFrameworkUtils

+ (void)prepareFramework
{
  Framework & f = GetFramework();
  MWMWatchLocationTracker * tracker = [MWMWatchLocationTracker sharedLocationTracker];
  CLLocationCoordinate2D coordinate = tracker.currentCoordinate;
  m2::PointD const current(MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude));
  f.SetViewportCenter(current);
  location::GpsInfo info = [tracker infoFromCurrentLocation];
  f.OnLocationUpdate(info);
}

+ (void)resetFramework
{
  //Workaround to reset framework on load new MWM from background app
  DeleteFramework();
}

+ (BOOL)hasMWM
{
  [MWMFrameworkUtils prepareFramework];
  Framework & f = GetFramework();
  storage::TIndex countryIndex = f.GetCountryIndex(f.GetViewportCenter());
  if (countryIndex == storage::TIndex())
    return f.IsCountryLoaded(f.GetViewportCenter());
  storage::TStatus countryStatus = f.GetCountryTree().GetActiveMapLayout().GetCountryStatus(countryIndex);
  return (countryStatus == storage::TStatus::EOnDisk || countryStatus == storage::TStatus::EOnDiskOutOfDate);
}

+ (NSString *)currentCountryName
{
  [MWMFrameworkUtils prepareFramework];
  Framework & f = GetFramework();
  storage::TIndex countryIndex = f.GetCountryIndex(f.GetViewportCenter());
  if (countryIndex == storage::TIndex())
    return @"";
  string countryName = f.GetCountryTree().GetActiveMapLayout().GetFormatedCountryName(countryIndex);
  return @(countryName.c_str());
}

+ (void)initSoftwareRenderer:(CGFloat)screenScale
{
  Framework & f = GetFramework();
  if (!f.IsSingleFrameRendererInited())
    f.InitSingleFrameRenderer(screenScale);
}

+ (void)releaseSoftwareRenderer
{
  GetFramework().ReleaseSingleFrameRenderer();
}

+ (UIImage *)getFrame:(CGSize)frameSize withScreenScale:(CGFloat)screenScale andZoomModifier:(int)zoomModifier
{
  [MWMFrameworkUtils prepareFramework];
  Framework & f = GetFramework();
  MWMWatchLocationTracker * tracker = [MWMWatchLocationTracker sharedLocationTracker];
  CLLocationCoordinate2D coordinate = tracker.currentCoordinate;

  m2::PointD const center = MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude);
  uint32_t const pxWidth = 2 * (uint32_t)frameSize.width;
  uint32_t const pxHeight = 2 * (uint32_t)frameSize.height;
  Framework::SingleFrameSymbols symbols;
  if (tracker.hasDestination)
  {
    symbols.m_showSearchResult = true;
    symbols.m_searchResult = tracker.destinationPosition;
    symbols.m_bottomZoom = 12;
  }
  else
    symbols.m_showSearchResult = false;


  Framework::FrameImage image;
  [MWMFrameworkUtils initSoftwareRenderer:screenScale];
  f.DrawSingleFrame(center, zoomModifier, pxWidth, pxHeight, image, symbols);
  NSData * imadeData = [NSData dataWithBytes:image.m_data.data() length:image.m_data.size()];
  return [UIImage imageWithData:imadeData];
}

+ (void)searchAroundCurrentLocation:(NSString *)query callback:(void(^)(NSMutableArray *result))reply
{
  [MWMFrameworkUtils prepareFramework];

  // Agreed to set 10km as search radius for Watch.
  search::SearchParams params;
  params.SetSearchRadiusMeters(10000.0);

  CLLocationCoordinate2D coordinate = [MWMWatchLocationTracker sharedLocationTracker].currentCoordinate;
  params.SetPosition(coordinate.latitude, coordinate.longitude);
  params.m_query = query.UTF8String;
  params.m_callback = ^(search::Results const & results)
  {
    if (results.IsEndMarker())
      return;
    size_t const maxResultsForWatch = MIN(results.GetCount(), 20);
    NSMutableArray * res = [NSMutableArray arrayWithCapacity:maxResultsForWatch];
    for (size_t index = 0; index < maxResultsForWatch; ++index)
    {
      search::Result const & result = results.GetResult(index);
      NSMutableDictionary * d = [NSMutableDictionary dictionary];
      d[kSearchResultTitleKey] = @(result.GetString());
      d[kSearchResultCategoryKey] = @(result.GetFeatureType());
      m2::PointD const featureCenter = result.GetFeatureCenter();
      d[kSearchResultPointKey] = [NSValue value:&featureCenter withObjCType:@encode(m2::PointD)];
      res[index] = d;
    }
    reply(res);
  };
  params.SetForceSearch(true);
  GetFramework().Search(params);
}

@end