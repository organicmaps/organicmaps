#import "MWMMapDownloaderAdsTableViewCell.h"

@implementation MWMMapDownloaderAdsTableViewCell

+ (CGFloat)estimatedHeight { return 68.0; }
- (void)setAdView:(MTRGAppwallBannerAdView *)view
{
  if (_adView == view)
    return;
  [_adView removeFromSuperview];
  [self addSubview:view];
  _adView = view;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.adView.frame = self.bounds;
}

@end
