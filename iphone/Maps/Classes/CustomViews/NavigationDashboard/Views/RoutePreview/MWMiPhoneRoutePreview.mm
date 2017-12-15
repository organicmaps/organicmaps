#import "MWMiPhoneRoutePreview.h"
#import "MWMCommon.h"
#import "SwiftBridge.h"

@interface MWMRoutePreview ()

@property(nonatomic) BOOL isVisible;

@end

@interface MWMiPhoneRoutePreview ()
@property(weak, nonatomic) IBOutlet UIButton * backButton;
@property(nonatomic) NSLayoutConstraint * verticalConstraint;
@end

@implementation MWMiPhoneRoutePreview

- (void)setupConstraints
{
  auto sv = self.superview;
  [self.leadingAnchor constraintEqualToAnchor:sv.leadingAnchor].active = YES;
  [self.trailingAnchor constraintEqualToAnchor:sv.trailingAnchor].active = YES;
  self.verticalConstraint = [self.bottomAnchor constraintEqualToAnchor:sv.topAnchor];
  self.verticalConstraint.active = YES;

  NSLayoutXAxisAnchor * backLeadingAnchor = sv.leadingAnchor;
  if (@available(iOS 11.0, *))
  {
    backLeadingAnchor = sv.safeAreaLayoutGuide.leadingAnchor;
  }
  [self.backButton.leadingAnchor constraintEqualToAnchor:backLeadingAnchor].active = YES;
}

- (void)setIsVisible:(BOOL)isVisible
{
  self.verticalConstraint.active = NO;
  auto sv = self.superview;
  NSLayoutYAxisAnchor * topAnchor = sv.topAnchor;
  NSLayoutYAxisAnchor * selfAnchor = isVisible ? self.topAnchor : self.bottomAnchor;
  CGFloat constant = 0;
  if (isVisible)
  {
    if (@available(iOS 11.0, *))
    {
      topAnchor = sv.safeAreaLayoutGuide.topAnchor;
    }
    else
    {
      constant = statusBarHeight();
    }
  }
  self.verticalConstraint = [selfAnchor constraintEqualToAnchor:topAnchor constant:constant];
  self.verticalConstraint.active = YES;
  [super setIsVisible:isVisible];
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
