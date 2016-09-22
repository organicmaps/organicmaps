#import "MWMMyTargetDelegate.h"

@interface MWMMyTarget : NSObject

+ (MWMMyTarget *)manager;
+ (void)startAdServerForbiddenCheckTimer;

@property(weak, nonatomic) id<MWMMyTargetDelegate> delegate;
@property(nonatomic, readonly) NSUInteger bannersCount;

- (MTRGNativeAppwallBanner *)bannerAtIndex:(NSUInteger)index;
- (void)handleBannerShowAtIndex:(NSUInteger)index;
- (void)handleBannerClickAtIndex:(NSUInteger)index withController:(UIViewController *)controller;

@end
