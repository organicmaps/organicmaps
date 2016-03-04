#import "MWMButton.h"

typedef NS_ENUM(NSInteger, MWMCircularProgressState)
{
  MWMCircularProgressStateNormal,
  MWMCircularProgressStateSelected,
  MWMCircularProgressStateProgress,
  MWMCircularProgressStateSpinner,
  MWMCircularProgressStateFailed,
  MWMCircularProgressStateCompleted
};

@class MWMCircularProgress;

@protocol MWMCircularProgressProtocol <NSObject>

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress : NSObject

+ (nonnull instancetype)downloaderProgressForParentView:(nonnull UIView *)parentView;

@property (nonatomic) CGFloat progress;
@property (nonatomic) MWMCircularProgressState state;
@property (weak, nonatomic) id<MWMCircularProgressProtocol> delegate;

- (void)setImage:(nonnull UIImage *)image forState:(MWMCircularProgressState)state;
- (void)setColor:(nonnull UIColor *)color forState:(MWMCircularProgressState)state;
- (void)setColoring:(MWMButtonColoring)coloring forState:(MWMCircularProgressState)state;
- (void)setInvertColor:(BOOL)invertColor;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView;

@end
