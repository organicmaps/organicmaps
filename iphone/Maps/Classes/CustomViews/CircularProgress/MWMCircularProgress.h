#import "MWMButton.h"
#import "MWMCircularProgressState.h"
#import "UIImageView+Coloring.h"

NS_ASSUME_NONNULL_BEGIN

@class MWMCircularProgress;

typedef NSArray<NSNumber *> *MWMCircularProgressStateVec;

@protocol MWMCircularProgressProtocol<NSObject>

- (void)progressButtonPressed:(MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress: NSObject<CAAnimationDelegate>

+ (instancetype)downloaderProgressForParentView:(UIView *)parentView;

@property(nonatomic) CGFloat progress;
@property(nonatomic) MWMCircularProgressState state;
@property(weak, nonatomic, nullable) id<MWMCircularProgressProtocol> delegate;

- (void)setSpinnerColoring:(MWMImageColoring)coloring;
- (void)setSpinnerBackgroundColor:(UIColor *)backgroundColor;
- (void)setInvertColor:(BOOL)invertColor;
- (void)setCancelButtonHidden;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(UIView *)parentView;

- (void)setImageName:(nullable NSString *)imageName
           forStates:(MWMCircularProgressStateVec)states;
- (void)setColor:(UIColor *)color forStates:(MWMCircularProgressStateVec)states;
- (void)setColoring:(MWMButtonColoring)coloring
          forStates:(MWMCircularProgressStateVec)states;
@end

NS_ASSUME_NONNULL_END
