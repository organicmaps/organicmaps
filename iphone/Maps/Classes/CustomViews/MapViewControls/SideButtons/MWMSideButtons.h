#include "platform/location.hpp"

@class MWMSideButtonsView;

@interface MWMSideButtons : NSObject

@property (nonatomic) BOOL zoomHidden;
@property (nonatomic) BOOL hidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view;
- (void)setTopBound:(CGFloat)bound;
- (void)setBottomBound:(CGFloat)bound;
- (void)mwm_refreshUI;

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode;

@end
