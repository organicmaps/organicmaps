#import <MyTargetSDK/MTRGAppwallBannerAdView.h>
#import "MWMMapDownloaderTableViewCellProtocol.h"
#import "MWMTableViewCell.h"

@interface MWMMapDownloaderAdsTableViewCell
    : MWMTableViewCell<MWMMapDownloaderTableViewCellProtocol>

@property(nonatomic) MTRGAppwallBannerAdView * adView;

@end
