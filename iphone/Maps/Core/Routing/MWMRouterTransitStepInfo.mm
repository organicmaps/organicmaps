#import "MWMRouterTransitStepInfo+Core.h"

#include "map/transit/transit_display.hpp"

#include "base/assert.hpp"

namespace
{
MWMRouterTransitType convertType(TransitType type)
{
  switch (type)
  {
  case TransitType::IntermediatePoint: return MWMRouterTransitTypeIntermediatePoint;
  case TransitType::Pedestrian: return MWMRouterTransitTypePedestrian;
  case TransitType::Subway: return MWMRouterTransitTypeSubway;
  case TransitType::Train: return MWMRouterTransitTypeTrain;
  case TransitType::LightRail: return MWMRouterTransitTypeLightRail;
  case TransitType::Monorail: return MWMRouterTransitTypeMonorail;
  case TransitType::Tram: return MWMRouterTransitTypeTram;
  case TransitType::Bus: return MWMRouterTransitTypeBus;
  case TransitType::Ferry: return MWMRouterTransitTypeFerry;
  case TransitType::CableTram: return MWMRouterTransitTypeCableTram;
  case TransitType::AerialLift: return MWMRouterTransitTypeAerialLift;
  case TransitType::Funicular: return MWMRouterTransitTypeFunicular;
  // Trolleybus stops are not visually distinguished from buses in known cities; reuse the bus icon.
  case TransitType::Trolleybus: return MWMRouterTransitTypeBus;
  case TransitType::AirService: return MWMRouterTransitTypeAirService;
  case TransitType::WaterService: return MWMRouterTransitTypeWaterService;
  }
  UNREACHABLE();
}

UIColor * convertColor(uint32_t colorARGB)
{
  CGFloat const alpha = CGFloat((colorARGB >> 24) & 0xFF) / 255;
  CGFloat const red = CGFloat((colorARGB >> 16) & 0xFF) / 255;
  CGFloat const green = CGFloat((colorARGB >> 8) & 0xFF) / 255;
  CGFloat const blue = CGFloat(colorARGB & 0xFF) / 255;
  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}
}  // namespace

@implementation MWMRouterTransitStepInfo

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    // Steps built on the iOS side (ruler routes) set only the fields their cells show.
    _distance = @"";
    _distanceUnits = @"";
    _number = @"";
    _color = UIColor.clearColor;
    _lineNumber = @"";
    _startStopName = @"";
    _endStopName = @"";
    _intermediateStopNames = @[];
  }
  return self;
}

- (instancetype)initWithStepInfo:(TransitStepInfo const &)info
{
  self = [super init];
  if (self)
  {
    _type = convertType(info.m_type);
    _distance = @(info.m_distanceStr.c_str());
    _distanceUnits = @(info.m_distanceUnitsSuffix.c_str());
    _number = @(info.m_number.c_str());
    _color = convertColor(info.m_colorARGB);
    _intermediateIndex = info.m_intermediateIndex;
    _timeInSec = info.m_timeInSec;
    _lineNumber = @(info.m_lineNumber.c_str());
    _startStopName = @(info.m_startStopName.c_str());
    _endStopName = @(info.m_endStopName.c_str());
    NSMutableArray<NSString *> * intermediates = [NSMutableArray arrayWithCapacity:info.m_intermediateStopNames.size()];
    for (auto const & name : info.m_intermediateStopNames)
      [intermediates addObject:@(name.c_str())];
    _intermediateStopNames = intermediates;
    _stopCount = info.m_stopCount;
  }
  return self;
}

@end
