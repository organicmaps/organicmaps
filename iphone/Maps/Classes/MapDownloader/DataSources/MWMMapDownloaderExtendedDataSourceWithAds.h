#import <MyTargetSDK/MTRGAppwallBannerAdView.h>
#import "MWMMapDownloaderExtendedDataSource.h"

@interface MWMMapDownloaderExtendedDataSourceWithAds : MWMMapDownloaderExtendedDataSource

- (MTRGAppwallBannerAdView *)viewForBannerAtIndex:(NSUInteger)index;

@end
