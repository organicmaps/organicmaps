#import <XCTest/XCTest.h>

#import "MWMRouterTransitStepInfo+Core.h"

#include "map/transit/transit_display.hpp"

@interface MWMRouterTransitStepInfoTests : XCTestCase
@end

@implementation MWMRouterTransitStepInfoTests

- (void)testSupportedTransitModesKeepTheirPresentationTypeAndMetadata
{
  struct Case
  {
    TransitType m_coreType;
    MWMRouterTransitType m_expectedType;
  };
  Case const cases[] = {
      {TransitType::IntermediatePoint, MWMRouterTransitTypeIntermediatePoint},
      {TransitType::Pedestrian, MWMRouterTransitTypePedestrian},
      {TransitType::Subway, MWMRouterTransitTypeSubway},
      {TransitType::Train, MWMRouterTransitTypeTrain},
      {TransitType::LightRail, MWMRouterTransitTypeLightRail},
      {TransitType::Monorail, MWMRouterTransitTypeMonorail},
      {TransitType::Tram, MWMRouterTransitTypeTram},
      {TransitType::Bus, MWMRouterTransitTypeBus},
      {TransitType::Ferry, MWMRouterTransitTypeFerry},
      {TransitType::CableTram, MWMRouterTransitTypeCableTram},
      {TransitType::AerialLift, MWMRouterTransitTypeAerialLift},
      {TransitType::Funicular, MWMRouterTransitTypeFunicular},
      {TransitType::Trolleybus, MWMRouterTransitTypeBus},
      {TransitType::AirService, MWMRouterTransitTypeAirService},
      {TransitType::WaterService, MWMRouterTransitTypeWaterService},
  };
  for (auto const & test : cases)
  {
    TransitStepInfo info(test.m_coreType, 42, 20, "12", 0xFF2F6499, 2);
    info.m_distanceStr = "42";
    info.m_distanceUnitsSuffix = "m";
    auto step = [[MWMRouterTransitStepInfo alloc] initWithStepInfo:info];

    XCTAssertEqual(step.type, test.m_expectedType, @"%s", DebugPrint(test.m_coreType).c_str());
    XCTAssertEqualObjects(step.number, @"12");
    XCTAssertEqualObjects(step.distance, @"42");
    XCTAssertEqualObjects(step.distanceUnits, @"m");
    XCTAssertEqual(step.intermediateIndex, 2);
  }
}

@end
