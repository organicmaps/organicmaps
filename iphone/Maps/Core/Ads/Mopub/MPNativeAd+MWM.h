#import "MPNativeAd.h"
#import "MPNativeView.h"

@interface MPNativeAd (MWM) <MPNativeViewDelegate>

- (void)setAdView:(UIView *)view;
- (void)setActionButtons:(NSArray<UIButton *> *)buttons;

@end
