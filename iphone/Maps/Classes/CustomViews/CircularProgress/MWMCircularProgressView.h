#import <UIKit/UIKit.h>

@interface MWMCircularProgressView : UIView

@property (nonatomic, readonly) BOOL animating;

- (nonnull instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (nonnull instancetype)init __attribute__((unavailable("init is not available")));

- (void)animateFromValue:(CGFloat)fromValue toValue:(CGFloat)toValue;

- (void)refreshProgress;
- (void)updatePath:(CGFloat)progress;
- (void)startSpinner;
- (void)stopSpinner;

@end
