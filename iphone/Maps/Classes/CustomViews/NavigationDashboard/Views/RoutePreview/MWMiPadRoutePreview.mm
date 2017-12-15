#import "MWMiPadRoutePreview.h"
#import "MWMRouter.h"
#import "SwiftBridge.h"

@interface MWMRoutePreview ()

@property(nonatomic) BOOL isVisible;

@end

@interface MWMiPadRoutePreview ()
@property(nonatomic) NSLayoutConstraint * horizontalConstraint;
@end

@implementation MWMiPadRoutePreview

- (void)setupConstraints
{
  auto sv = self.superview;
  [self.topAnchor constraintEqualToAnchor:sv.topAnchor].active = YES;
  [self.bottomAnchor constraintEqualToAnchor:sv.bottomAnchor].active = YES;
  self.horizontalConstraint = [self.trailingAnchor constraintEqualToAnchor:sv.leadingAnchor];
  self.horizontalConstraint.active = YES;
}

- (void)setIsVisible:(BOOL)isVisible
{
  self.horizontalConstraint.active = NO;
  auto sv = self.superview;
  NSLayoutXAxisAnchor * selfAnchor = isVisible ? self.leadingAnchor : self.trailingAnchor;
  self.horizontalConstraint = [selfAnchor constraintEqualToAnchor:sv.leadingAnchor];
  self.horizontalConstraint.active = YES;
  [super setIsVisible:isVisible];
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
