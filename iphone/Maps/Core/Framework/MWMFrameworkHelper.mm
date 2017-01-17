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
  CGFloat const x0 = rect.origin.x * scale;
  CGFloat const y0 = rect.origin.y * scale;
  CGFloat const x1 = x0 + rect.size.width * scale;
  CGFloat const y1 = y0 + rect.size.height * scale;
  GetFramework().SetVisibleViewport(m2::RectD(x0, y0, x1, y1));
}

@end
