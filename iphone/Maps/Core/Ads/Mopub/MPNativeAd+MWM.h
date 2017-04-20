#import "MPNativeAd.h"
#import "MPNativeView.h"

@interface MPNativeAd (MWM) <MPNativeViewDelegate>

- (void)setAdView:(UIView *)view actionButtons:(NSArray<UIButton *> *)buttons;
- (void)unregister;

@end
