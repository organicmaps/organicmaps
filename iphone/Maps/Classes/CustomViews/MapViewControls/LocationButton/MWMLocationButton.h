#import <Foundation/Foundation.h>

@interface MWMLocationButton : NSObject

@property (nonatomic) BOOL hidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view;

@end
