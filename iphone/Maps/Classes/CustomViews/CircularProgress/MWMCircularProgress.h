#import <Foundation/Foundation.h>

@class MWMCircularProgress;

@protocol MWMCircularProgressDelegate <NSObject>

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress;

@end

@interface MWMCircularProgress : NSObject

@property (nonatomic) CGFloat progress;
@property (nonatomic) BOOL failed;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMCircularProgressDelegate>)delegate;
- (void)reset;

@end
