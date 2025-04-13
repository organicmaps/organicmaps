#import "MWMMyPositionMode.h"

@interface MWMSideButtons : NSObject

+ (MWMSideButtons *)buttons;

@property (nonatomic) BOOL zoomHidden;
@property (nonatomic) BOOL hidden;
@property (nonatomic, readonly) UIView *view;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view;

- (void)processMyPositionStateModeEvent:(MWMMyPositionMode)mode;

+ (void)updateAvailableArea:(CGRect)frame;

@end
