@protocol DownloadIndicatorProtocol <NSObject>

- (void)enableStandby;
- (void)disableStandby;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

@end
