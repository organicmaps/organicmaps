#import "SearchResult+Core.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "SwiftBridge.h"

#import "platform/localization.hpp"
#import "platform/distance.hpp"

#include "map/bookmark_helpers.hpp"

#import "geometry/mercator.hpp"

@implementation SearchResult

- (instancetype)initWithTitleText:(NSString *)titleText type:(SearchItemType)type suggestion:(NSString *)suggestion {
  self = [super init];
  if (self) {
    _titleText = titleText;
    _itemType = type;
    _suggestion = suggestion;
  };
  return self;
}

@end

@implementation SearchResult(Core)

- (instancetype)initWithResult:(const search::Result &)result itemType:(SearchItemType)itemType index:(NSUInteger)index {
  self = [super init];
  if (self) {
    _index = index;
    _titleText = result.GetString().empty() ? @(result.GetLocalizedFeatureType().c_str()) : @(result.GetString().c_str());
    _addressText = @(result.GetAddress().c_str());
    _infoText = @(result.GetFeatureDescription().c_str());
    if (result.IsSuggest())
      _suggestion = @(result.GetSuggestionString().c_str());

    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    if (lastLocation && result.HasPoint()) {
      double distanceInMeters = mercator::DistanceOnEarth(lastLocation.mercator, result.GetFeatureCenter());
      std::string distanceStr = platform::Distance::CreateFormatted(distanceInMeters).ToString();
      _distanceText = @(distanceStr.c_str());

      auto const & pivot = result.GetFeatureCenter();
      _point = CGPointMake(pivot.x, pivot.y);
      auto const location = mercator::ToLatLon(pivot);
      _coordinate = CLLocationCoordinate2DMake(location.m_lat, location.m_lon);
    } else {
      _distanceText = nil;
    }

    switch (result.IsOpenNow()) {
      case osm::Yes: {
        const int minutes = result.GetMinutesUntilClosed();
        if (minutes < 60) { // less than 1 hour
          _openStatusColor = UIColor.systemYellowColor;
          NSString * time = [NSString stringWithFormat:@"%d %@", minutes, L(@"minute")];
          _openStatusText = [NSString stringWithFormat:L(@"closes_in"), time];
        } else {
          _openStatusColor = UIColor.systemGreenColor;
          _openStatusText = L(@"editor_time_open");
        }
        break;
      }
      case osm::No: {
        const int minutes = result.GetMinutesUntilOpen();
        if (minutes < 60) { // less than 1 hour
          NSString * time = [NSString stringWithFormat:@"%d %@", minutes, L(@"minute")];
          _openStatusText = [NSString stringWithFormat:L(@"opens_in"), time];
        } else {
          _openStatusText = L(@"closed");
        }
        _openStatusColor = UIColor.systemRedColor;
        break;
      }
      case osm::Unknown: {
        _openStatusText = nil;
        _openStatusColor = UIColor.clearColor;
        break;
      }
    }

    _isPopularHidden = YES; // Restore logic in the future when popularity is available.
    _isPureSuggest = result.GetResultType() == search::Result::Type::PureSuggest;

    NSMutableArray<NSValue *> * ranges = [NSMutableArray array];
    size_t const rangesCount = result.GetHighlightRangesCount();
    for (size_t i = 0; i < rangesCount; ++i) {
      auto const &range = result.GetHighlightRange(i);
      NSRange nsRange = NSMakeRange(range.first, range.second);
      [ranges addObject:[NSValue valueWithRange:nsRange]];
    }
    _highlightRanges = [ranges copy];

    _itemType = itemType;

    if (result.GetResultType() == search::Result::Type::Feature) {
      auto const featureType = result.GetFeatureType();
      auto const bookmarkImage = GetBookmarkIconByFeatureType(featureType);
      _iconImageName = [NSString stringWithFormat:@"%@%@",
                        @"ic_bm_",
                        [@(kml::ToString(bookmarkImage).c_str()) lowercaseString]];
    }
  }
  return self;
}

@end
