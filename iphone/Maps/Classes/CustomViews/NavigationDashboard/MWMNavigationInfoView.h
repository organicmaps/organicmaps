#import "MWMNavigationDashboardInfoProtocol.h"

@interface MWMNavigationInfoView : UIView<MWMNavigationDashboardInfoProtocol>

@property(nonatomic) CGFloat leftBound;
@property(nonatomic, readonly) CGFloat extraCompassBottomOffset;
@property(nonatomic, readonly) CGFloat visibleHeight;

- (void)addToView:(UIView *)superview;
- (void)remove;

@end
