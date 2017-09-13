#import "MWMButton.h"
#import "MWMCircularProgress.h"

@interface MWMCircularProgressView : UIView

@property(nonatomic, readonly) BOOL animating;

@property(nonatomic) MWMCircularProgressState state;
@property(nonatomic) BOOL isInvertColor;

- (nonnull instancetype)initWithFrame:(CGRect)frame
    __attribute__((unavailable("initWithFrame is not available")));
- (nonnull instancetype)init __attribute__((unavailable("init is not available")));

- (void)setSpinnerColoring:(MWMImageColoring)coloring;
- (void)setSpinnerBackgroundColor:(nonnull UIColor *)backgroundColor;
- (void)setImageName:(nullable NSString *)imageName forState:(MWMCircularProgressState)state;
- (void)setColor:(nonnull UIColor *)color forState:(MWMCircularProgressState)state;
- (void)setColoring:(MWMButtonColoring)coloring forState:(MWMCircularProgressState)state;

- (void)animateFromValue:(CGFloat)fromValue toValue:(CGFloat)toValue;

- (void)updatePath:(CGFloat)progress;

@end
