#import <Foundation/Foundation.h>

@interface MWMZoomButtons : NSObject

@property (nonatomic) BOOL hidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view;
- (void)setTopBound:(CGFloat)bound;
- (void)setBottomBound:(CGFloat)bound;

@end
