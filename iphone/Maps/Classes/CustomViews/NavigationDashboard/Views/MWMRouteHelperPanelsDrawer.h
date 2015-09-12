#import "MWMRouteHelperPanel.h"

@interface MWMRouteHelperPanelsDrawer : NSObject

@property (weak, nonatomic, readonly) UIView * parentView;

- (instancetype)initWithView:(UIView *)view;
- (void)drawPanels:(NSArray *)panels;
- (void)invalidateTopBounds:(NSArray *)panels forOrientation:(UIInterfaceOrientation)orientation;

@end
