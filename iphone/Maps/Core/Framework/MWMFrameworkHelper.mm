#import "MWMFrameworkHelper.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "MapViewController.h"

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

+ (void)setVisibleViewport:(CGRect)rect
{
  CGFloat const scale = [MapViewController controller].view.contentScaleFactor;
  CGFloat const x1 = rect.origin.x * scale;
  CGFloat const y1 = rect.origin.y * scale;
  CGFloat const x2 = x1 + rect.size.width * scale;
  CGFloat const y2 = y1 + rect.size.height * scale;
  GetFramework().SetVisibleViewport(m2::RectD(x1, y1, x2, y2));
}

@end
