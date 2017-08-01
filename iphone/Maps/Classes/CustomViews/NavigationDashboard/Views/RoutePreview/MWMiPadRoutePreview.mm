#import "MWMiPadRoutePreview.h"
#import "MWMRouter.h"
#import "SwiftBridge.h"

@interface MWMRoutePreview ()

@property(nonatomic) BOOL isVisible;

@end

@implementation MWMiPadRoutePreview

- (CGRect)defaultFrame
{
  CGFloat const width = 320;
  return {{self.isVisible ? 0 : -width, 0}, {width, self.superview.height}};
}

#pragma mark - SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}
#pragma mark - AvailableArea / VisibleArea

- (MWMAvailableAreaAffectDirections)visibleAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsLeft;
}

#pragma mark - AvailableArea / PlacePageArea

- (MWMAvailableAreaAffectDirections)placePageAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsLeft;
}

#pragma mark - AvailableArea / WidgetsArea

- (MWMAvailableAreaAffectDirections)widgetsAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsLeft;
}

#pragma mark - AvailableArea / SideButtonsArea

- (MWMAvailableAreaAffectDirections)sideButtonsAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsLeft;
}

#pragma mark - AvailableArea / TrafficButtonArea

- (MWMAvailableAreaAffectDirections)trafficButtonAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsLeft;
}

#pragma mark - AvailableArea / NavigationInfoArea

- (MWMAvailableAreaAffectDirections)navigationInfoAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsLeft;
}

@end
