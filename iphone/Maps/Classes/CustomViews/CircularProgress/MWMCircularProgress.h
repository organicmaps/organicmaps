#import <Foundation/Foundation.h>

@class MWMCircularProgress;

@protocol MWMCircularProgressDelegate <NSObject>

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress : NSObject

@property (nonatomic) CGFloat progress;
@property (nonatomic) BOOL failed;
@property (nonatomic) BOOL selected;

- (void)setImage:(nullable UIImage *)image forState:(UIControlState)state;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMCircularProgressDelegate>)delegate;
- (void)reset;
- (void)startSpinner;
- (void)stopSpinner;

@end
