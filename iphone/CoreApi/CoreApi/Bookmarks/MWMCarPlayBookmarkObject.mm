#import "MWMCarPlayBookmarkObject.h"
#include "Framework.h"
#include "geometry/mercator.hpp"

@interface MWMCarPlayBookmarkObject ()
@property(assign, nonatomic, readwrite) MWMMarkID bookmarkId;
@property(strong, nonatomic, readwrite) NSString * prefferedName;
@property(strong, nonatomic, readwrite) NSString * address;
@property(assign, nonatomic, readwrite) CLLocationCoordinate2D coordinate;
@property(assign, nonatomic, readwrite) CGPoint mercatorPoint;
@end

@implementation MWMCarPlayBookmarkObject

- (nullable instancetype)initWithBookmarkId:(MWMMarkID)bookmarkId
{
  self = [super init];
  if (!self)
    return nil;

  // The id may outlive the bookmark, for example when a CarPlay list still shows a bookmark deleted meanwhile.
  Bookmark const * bookmark = GetFramework().GetBookmarkManager().GetBookmark(bookmarkId);
  if (!bookmark)
    return nil;

  self.bookmarkId = bookmarkId;
  self.prefferedName = @(bookmark->GetPreferredName().c_str());
  auto const pivot = bookmark->GetPivot();
  self.mercatorPoint = CGPointMake(pivot.x, pivot.y);
  auto const & address = GetFramework().GetAddressAtPoint(pivot);
  self.address = @(address.FormatAddress().c_str());
  auto const location = mercator::ToLatLon(pivot);
  self.coordinate = CLLocationCoordinate2DMake(location.m_lat, location.m_lon);
  return self;
}
@end
