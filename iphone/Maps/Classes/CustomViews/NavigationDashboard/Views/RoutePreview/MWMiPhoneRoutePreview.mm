#import "MWMiPhoneRoutePreview.h"
#import "SwiftBridge.h"

@interface MWMRoutePreview ()

@property(nonatomic) BOOL isVisible;

@end

@implementation MWMiPhoneRoutePreview

- (CGRect)defaultFrame
{
  CGFloat const height = 64;
  return {{0, self.isVisible ? 0 : -height}, {self.superview.width, height}};
}

#pragma mark - AvailableArea / VisibleArea

- (MWMAvailableAreaAffectDirections)visibleAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsTop;
}

#pragma mark - AvailableArea / SideButtonsArea

- (MWMAvailableAreaAffectDirections)sideButtonsAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsTop;
}

#pragma mark - AvailableArea / TrafficButtonArea

- (MWMAvailableAreaAffectDirections)trafficButtonAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsTop;
}

@end
