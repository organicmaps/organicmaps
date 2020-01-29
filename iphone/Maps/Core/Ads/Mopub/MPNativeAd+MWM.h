#import "FacebookNativeAdAdapter.h"
#import "MPNativeAd.h"
#import "MPNativeView.h"

@class FBNativeAd;

@interface MPNativeAd (MWM) <MPNativeViewDelegate>

- (void)setAdView:(UIView *)view actionButtons:(NSArray<UIButton *> *)buttons;
- (void)unregister;

@property(nonatomic, readonly) id<MPNativeAdAdapter> adAdapter;

@end

@interface FacebookNativeAdAdapter ()

@property(nonatomic, readonly) FBNativeAd * fbNativeAd;

@end
