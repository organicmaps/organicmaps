#import "MWMButton.h"
#import "MWMCircularProgressState.h"
#import "UIImageView+Coloring.h"

#include "std/vector.hpp"

using MWMCircularProgressStateVec = vector<MWMCircularProgressState>;

@class MWMCircularProgress;

@protocol MWMCircularProgressProtocol<NSObject>

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress : NSObject <CAAnimationDelegate>

+ (nonnull instancetype)downloaderProgressForParentView:(nonnull UIView *)parentView;

@property(nonatomic) CGFloat progress;
@property(nonatomic) MWMCircularProgressState state;
@property(weak, nonatomic) id<MWMCircularProgressProtocol> _Nullable delegate;

- (void)setSpinnerColoring:(MWMImageColoring)coloring;
- (void)setSpinnerBackgroundColor:(nonnull UIColor *)backgroundColor;
- (void)setImageName:(nullable NSString *)imageName
           forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColor:(nonnull UIColor *)color forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColoring:(MWMButtonColoring)coloring
          forStates:(MWMCircularProgressStateVec const &)states;
- (void)setInvertColor:(BOOL)invertColor;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView;

@end
