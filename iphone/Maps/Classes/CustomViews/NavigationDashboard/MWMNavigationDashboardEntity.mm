#import "MWMNavigationDashboardEntity.h"
#import "MWMCoreUnits.h"
#import "MWMLocationManager.h"
#import "MWMRouterTransitStepInfo.h"
#import "MWMSettings.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "map/routing_manager.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

@implementation MWMNavigationDashboardEntity

- (BOOL)isSpeedCamLimitExceeded
{
  return GetFramework().GetRoutingManager().IsSpeedCamLimitExceeded();
}

- (NSString *)eta { return [NSDateComponentsFormatter etaStringFrom:self.timeToTarget]; }
- (NSString *)arrival
{
  auto arrivalDate = [[NSDate date] dateByAddingTimeInterval:self.timeToTarget];
  return [NSDateFormatter localizedStringFromDate:arrivalDate
                                        dateStyle:NSDateFormatterNoStyle
                                        timeStyle:NSDateFormatterShortStyle];
}

- (NSAttributedString *)estimateDot
{
  auto attributes = @{
    NSForegroundColorAttributeName: [UIColor blackSecondaryText],
    NSFontAttributeName: [UIFont medium17]
  };
  return [[NSAttributedString alloc] initWithString:@" • " attributes:attributes];
}

@end
