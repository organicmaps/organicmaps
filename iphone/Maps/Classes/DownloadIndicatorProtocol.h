#import <Foundation/Foundation.h>

@protocol DownloadIndicatorProtocol <NSObject>

- (void)enableStandby;
- (void)disableStandby;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

@end
