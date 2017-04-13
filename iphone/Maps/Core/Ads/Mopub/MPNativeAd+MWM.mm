#import "MPNativeAd+MWM.h"
#import "SwiftBridge.h"

@interface MPNativeAd ()

@property (nonatomic) MPNativeView * associatedView;
@property (nonatomic) BOOL hasAttachedToView;

- (void)willAttachToView:(UIView *)view;
- (void)adViewTapped;
- (void)nativeViewWillMoveToSuperview:(UIView *)superview;

@end


@implementation MPNativeAd (MWM)

- (void)setAdView:(UIView *)view
{
  self.associatedView = static_cast<MPNativeView *>(view);
  static_cast<MWMAdBanner *>(view).mpNativeAd = self;
  if (!self.hasAttachedToView) {
    [self willAttachToView:self.associatedView];
    self.hasAttachedToView = YES;
  }
}

- (void)setActionButtons:(NSArray<UIButton *> *)buttons
{
  for (UIButton * button in buttons)
    [button addTarget:self action:@selector(adViewTapped) forControlEvents:UIControlEventTouchUpInside];
}

@end
