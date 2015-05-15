#import <Foundation/Foundation.h>

@protocol DownloadIndicatorProtocol <NSObject>

- (void)disableStandby;
- (void)enableStandby;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

@end
