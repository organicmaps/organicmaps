#import "MWMRouteHelperPanel.h"

@interface MWMRouteHelperPanelsDrawer : NSObject

@property (weak, nonatomic, readonly) UIView * topView;

- (instancetype)initWithTopView:(UIView *)view;
- (void)invalidateTopBounds:(NSArray *)panels topView:(UIView *)view;

@end
