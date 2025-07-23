#import "MWMRoutePoint.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "MWMRoutePoint+CPP.h"

#include "geometry/mercator.hpp"

#include "platform/measurement_utils.hpp"

@interface MWMRoutePoint ()

@property(nonatomic, readonly) m2::PointD point;

@end

@implementation MWMRoutePoint

- (instancetype)initWithLastLocationAndType:(MWMRoutePointType)type intermediateIndex:(size_t)intermediateIndex
{
  auto lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return nil;

  self = [super init];
  if (self)
  {
    _point = lastLocation.mercator;
    _title = L(@"p2p_your_location");
    _subtitle = @"";
    _isMyPosition = YES;
    _type = type;
    _intermediateIndex = intermediateIndex;

    [self validatePoint];
  }
  return self;
}

- (instancetype)initWithURLSchemeRoutePoint:(url_scheme::RoutePoint const &)point
                                       type:(MWMRoutePointType)type
                          intermediateIndex:(size_t)intermediateIndex
{
  self = [super init];
  if (self)
  {
    _point = point.m_org;
    _title = @(point.m_name.c_str());
    _subtitle = @"";
    _isMyPosition = NO;
    _type = type;
    _intermediateIndex = intermediateIndex;

    [self validatePoint];
  }
  return self;
}

- (instancetype)initWithRouteMarkData:(RouteMarkData const &)point
{
  self = [super init];
  if (self)
  {
    _point = point.m_position;
    _title = @(point.m_title.c_str());
    _subtitle = @(point.m_subTitle.c_str());
    _isMyPosition = point.m_isMyPosition;
    _intermediateIndex = point.m_intermediateIndex;
    switch (point.m_pointType)
    {
    case RouteMarkType::Start: _type = MWMRoutePointTypeStart; break;
    case RouteMarkType::Intermediate: _type = MWMRoutePointTypeIntermediate; break;
    case RouteMarkType::Finish: _type = MWMRoutePointTypeFinish; break;
    }

    [self validatePoint];
  }
  return self;
}

- (instancetype)initWithCGPoint:(CGPoint)point
                          title:(NSString *)title
                       subtitle:(NSString *)subtitle
                           type:(MWMRoutePointType)type
              intermediateIndex:(size_t)intermediateIndex
{
  auto const pointD = m2::PointD(point.x, point.y);
  self = [self initWithPoint:pointD title:title subtitle:subtitle type:type intermediateIndex:intermediateIndex];
  return self;
}

- (instancetype)initWithPoint:(m2::PointD const &)point
                        title:(NSString *)title
                     subtitle:(NSString *)subtitle
                         type:(MWMRoutePointType)type
            intermediateIndex:(size_t)intermediateIndex
{
  self = [super init];
  if (self)
  {
    _point = point;
    _title = title;
    _subtitle = subtitle ?: @"";
    _isMyPosition = NO;
    _type = type;
    _intermediateIndex = intermediateIndex;

    [self validatePoint];
  }
  return self;
}

- (void)validatePoint
{
  // Sync with RoutePointsLayout::kMaxIntermediatePointsCount constant.
  NSAssert(_intermediateIndex >= 0 && _intermediateIndex <= 100, @"Invalid intermediateIndex");
}

- (double)latitude
{
  return mercator::YToLat(self.point.y);
}
- (double)longitude
{
  return mercator::XToLon(self.point.x);
}

- (NSString *)latLonString
{
  return @(measurement_utils::FormatLatLon(self.latitude, self.longitude, true).c_str());
}

- (RouteMarkData)routeMarkData
{
  [self validatePoint];

  RouteMarkData pt;
  switch (self.type)
  {
  case MWMRoutePointTypeStart: pt.m_pointType = RouteMarkType::Start; break;
  case MWMRoutePointTypeIntermediate: pt.m_pointType = RouteMarkType::Intermediate; break;
  case MWMRoutePointTypeFinish: pt.m_pointType = RouteMarkType::Finish; break;
  }
  pt.m_position = self.point;
  pt.m_isMyPosition = self.isMyPosition;
  pt.m_title = self.title.UTF8String;
  pt.m_subTitle = self.subtitle.UTF8String;
  pt.m_intermediateIndex = self.intermediateIndex;
  return pt;
}

- (NSString *)debugDescription
{
  NSString * type = nil;
  switch (_type)
  {
  case MWMRoutePointTypeStart: type = @"Start"; break;
  case MWMRoutePointTypeIntermediate: type = @"Intermediate"; break;
  case MWMRoutePointTypeFinish: type = @"Finish"; break;
  }

  return [NSString stringWithFormat:@"<%@: %p> Position: [%@, %@] | IsMyPosition: %@ | Type: %@ | "
                                    @"IntermediateIndex: %@ | Title: %@ | Subtitle: %@",
                                    [self class], self, @(_point.x), @(_point.y), _isMyPosition ? @"true" : @"false",
                                    type, @(_intermediateIndex), _title, _subtitle];
}

@end
