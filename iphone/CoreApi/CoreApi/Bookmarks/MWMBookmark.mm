#import "MWMBookmark+Core.h"
#import "MWMBookmarkColor+Core.h"

@implementation MWMBookmark

@end

@implementation MWMBookmark (Core)

- (instancetype)initWithMarkId:(MWMMarkID)markId bookmarkData:(Bookmark const *)bookmark
{
  self = [super init];
  if (self)
  {
    _bookmarkId = markId;
    _bookmarkName = @(bookmark->GetPreferredName().c_str());
    _bookmarkColor = convertKmlColor(bookmark->GetColor());
    _bookmarkIconName = [NSString
        stringWithFormat:@"%@%@", @"ic_bm_", [@(kml::ToString(bookmark->GetData().m_icon).c_str()) lowercaseString]];
    auto const & types = bookmark->GetData().m_featureTypes;
    if (!types.empty())
      _bookmarkType = @(kml::GetLocalizedFeatureType(types).c_str());
    auto latlon = bookmark->GetLatLon();
    _locationCoordinate = CLLocationCoordinate2DMake(latlon.m_lat, latlon.m_lon);
  }
  return self;
}

@end
