#import "FacebookNativeAdAdapter.h"
#import "MPNativeAd.h"
#import "MPNativeView.h"

@class FBNativeAd;
@class FBMediaView;

@interface MPNativeAd (MWM) <MPNativeViewDelegate>

- (void)setAdView:(UIView *)view iconView:(UIImageView *)iconImageView actionButtons:(NSArray<UIButton *> *)buttons;
- (void)unregister;

@property(nonatomic, readonly) id<MPNativeAdAdapter> adAdapter;

@end
