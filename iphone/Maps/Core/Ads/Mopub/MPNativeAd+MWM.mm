#import "FacebookNativeAdAdapter.h"
#import "MPNativeAd+MWM.h"
#import "SwiftBridge.h"

@interface MPNativeAd ()

@property(nonatomic) MPNativeView * associatedView;
@property(nonatomic) BOOL hasAttachedToView;
@property(nonatomic, readonly) id<MPNativeAdAdapter> adAdapter;

- (void)willAttachToView:(UIView *)view withAdContentViews:(NSArray *)adContentViews;
- (void)adViewTapped;
- (void)nativeViewWillMoveToSuperview:(UIView *)superview;
- (UIViewController *)viewControllerForPresentingModalView;

@end

@interface FacebookNativeAdAdapter ()

@property(nonatomic, readonly) FBNativeAd * fbNativeAd;

@end

@implementation MPNativeAd (MWM)

- (void)setAdView:(UIView *)view actionButtons:(NSArray<UIButton *> *)buttons
{
  self.associatedView = static_cast<MPNativeView *>(view);
  static_cast<MWMAdBanner *>(view).mpNativeAd = self;
  if (!self.hasAttachedToView) {
    auto adapter = self.adAdapter;
    if ([adapter isKindOfClass:[FacebookNativeAdAdapter class]])
    {
      auto fbAdapter = static_cast<FacebookNativeAdAdapter *>(adapter);
      [fbAdapter.fbNativeAd registerViewForInteraction:self.associatedView
                                    withViewController:[self viewControllerForPresentingModalView]
                                    withClickableViews:buttons];
    }
    else
    {
      [self willAttachToView:self.associatedView withAdContentViews:self.associatedView.subviews];
      for (UIButton * button in buttons)
      {
        [button addTarget:self
                      action:@selector(adViewTapped)
            forControlEvents:UIControlEventTouchUpInside];
      }
    }
    self.hasAttachedToView = YES;
  }
}

- (void)unregister
{
  auto adapter = self.adAdapter;
  self.delegate = nil;
  [self nativeViewWillMoveToSuperview:nil];
  self.associatedView = nil;
  self.hasAttachedToView = NO;
  if ([adapter isKindOfClass:[FacebookNativeAdAdapter class]])
  {
    auto fbAdapter = static_cast<FacebookNativeAdAdapter *>(adapter);
    [fbAdapter.fbNativeAd unregisterView];
  }
}

@end
