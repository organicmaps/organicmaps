#import "MWMNavigationDashboardInfoProtocol.h"

@interface MWMNavigationInfoView : UIView<MWMNavigationDashboardInfoProtocol>

@property(nonatomic, readonly) BOOL isVisible;
@property(nonatomic, readonly) CGFloat visibleHeight;

- (void)addToView:(UIView *)superview;
- (void)remove;

@end
