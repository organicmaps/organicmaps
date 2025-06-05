#import "MWMCarPlaySearchResultObject.h"
#import "MWMSearch.h"
#import "SearchResult.h"
#import "SwiftBridge.h"

#include "search/result.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"

@interface MWMCarPlaySearchResultObject ()
@property(assign, nonatomic, readwrite) NSInteger originalRow;
@property(strong, nonatomic, readwrite) NSString * title;
@property(strong, nonatomic, readwrite) NSString * address;
@property(assign, nonatomic, readwrite) CLLocationCoordinate2D coordinate;
@property(assign, nonatomic, readwrite) CGPoint mercatorPoint;
@end

@implementation MWMCarPlaySearchResultObject

- (instancetype)initForRow:(NSInteger)row
{
  self = [super init];
  if (self)
  {
    self.originalRow = row;
    NSInteger containerIndex = [MWMSearch containerIndexWithRow:row];
    SearchItemType type = [MWMSearch resultTypeWithRow:row];
    if (type == SearchItemTypeRegular)
    {
      auto const & result = [MWMSearch resultWithContainerIndex:containerIndex];
      self.title = result.titleText;
      self.address = result.addressText;
      self.coordinate = result.coordinate;
      auto const pivot = mercator::FromLatLon(result.coordinate.latitude, result.coordinate.longitude);
      self.mercatorPoint = CGPointMake(pivot.x, pivot.y);
      return self;
    }
  }
  return nil;
}

@end
