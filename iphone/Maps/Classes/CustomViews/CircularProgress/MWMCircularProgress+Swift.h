#import "MWMButton.h"
#import "MWMCircularProgressState.h"
#import "UIImageView+Coloring.h"

@class MWMCircularProgress;

@protocol MWMCircularProgressProtocol<NSObject>

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress : NSObject<CAAnimationDelegate>

+ (nonnull instancetype)downloaderProgressForParentView:(nonnull UIView *)parentView;

@property(nonatomic) CGFloat progress;
@property(nonatomic) MWMCircularProgressState state;
@property(weak, nonatomic) id<MWMCircularProgressProtocol> _Nullable delegate;

- (void)setSpinnerColoring:(MWMImageColoring)coloring;
- (void)setSpinnerBackgroundColor:(nonnull UIColor *)backgroundColor;
- (void)setInvertColor:(BOOL)invertColor;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView;

@end
