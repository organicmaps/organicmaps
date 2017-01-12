#import "MWMFrameworkHelper.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"

#include "Framework.h"

@implementation MWMFrameworkHelper

+ (void)zoomToCurrentPosition
{
  auto & f = GetFramework();
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    f.SwitchMyPositionNextMode();
  else
    f.SetViewportCenter(lastLocation.mercator, 13 /* zoom */);
}

@end
