
#import <Foundation/Foundation.h>

@protocol SearchActivityProtocol <NSObject>

- (void)setActive:(BOOL)active animated:(BOOL)animated;
@property (nonatomic, readonly) BOOL active;

@end
