#import "MWMButton.h"

#include "std/vector.hpp"

typedef NS_ENUM(NSInteger, MWMCircularProgressState)
{
  MWMCircularProgressStateNormal,
  MWMCircularProgressStateSelected,
  MWMCircularProgressStateProgress,
  MWMCircularProgressStateSpinner,
  MWMCircularProgressStateFailed,
  MWMCircularProgressStateCompleted
};

using MWMCircularProgressStateVec = vector<MWMCircularProgressState>;

@class MWMCircularProgress;

@protocol MWMCircularProgressProtocol <NSObject>

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress : NSObject

+ (nonnull instancetype)downloaderProgressForParentView:(nonnull UIView *)parentView;

@property (nonatomic) CGFloat progress;
@property (nonatomic) MWMCircularProgressState state;
@property (weak, nonatomic) id<MWMCircularProgressProtocol> _Nullable delegate;

- (void)setImage:(nonnull UIImage *)image forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColor:(nonnull UIColor *)color forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColoring:(MWMButtonColoring)coloring forStates:(MWMCircularProgressStateVec const &)states;
- (void)setInvertColor:(BOOL)invertColor;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView;

@end
