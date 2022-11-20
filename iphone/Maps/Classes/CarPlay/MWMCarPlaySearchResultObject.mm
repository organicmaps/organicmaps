#import "MWMCarPlaySearchResultObject.h"
#import "MWMSearch.h"

#include "search/result.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"

@interface MWMCarPlaySearchResultObject()
@property(assign, nonatomic, readwrite) NSInteger originalRow;
@property(strong, nonatomic, readwrite) NSString *title;
@property(strong, nonatomic, readwrite) NSString *address;
@property(assign, nonatomic, readwrite) CLLocationCoordinate2D coordinate;
@property(assign, nonatomic, readwrite) CGPoint mercatorPoint;
@end

@implementation MWMCarPlaySearchResultObject

- (instancetype)initForRow:(NSInteger)row {
  self = [super init];
  if (self) {
    self.originalRow = row;
    NSInteger containerIndex = [MWMSearch containerIndexWithRow:row];
    MWMSearchItemType type = [MWMSearch resultTypeWithRow:row];
    if (type == MWMSearchItemTypeRegular) {
      auto const & result = [MWMSearch resultWithContainerIndex:containerIndex];
      NSString *localizedTypeName = @"";
      if (result.GetResultType() == search::Result::Type::Feature) {
        auto const readableType = classif().GetReadableObjectName(result.GetFeatureType());
        localizedTypeName = @(platform::GetLocalizedTypeName(readableType).c_str());
      }
      self.title = result.GetString().empty() ? localizedTypeName : @(result.GetString().c_str());
      self.address = @(result.GetAddress().c_str());
      auto const pivot = result.GetFeatureCenter();
      self.mercatorPoint = CGPointMake(pivot.x, pivot.y);
      auto const location = mercator::ToLatLon(pivot);
      self.coordinate = CLLocationCoordinate2DMake(location.m_lat, location.m_lon);
      return self;
    }
  }
  return nil;
}

@end
