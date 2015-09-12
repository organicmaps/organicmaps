#import "MWMRouteHelperPanel.h"

@interface MWMNextTurnPanel : MWMRouteHelperPanel

+ (instancetype)turnPanelWithOwnerView:(UIView *)ownerView;
- (void)configureWithImage:(UIImage *)image;

@end
