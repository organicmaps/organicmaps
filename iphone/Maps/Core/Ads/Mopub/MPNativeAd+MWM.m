#import <FBAudienceNetwork/FBNativeAd.h>
#import "MPNativeAd+MWM.h"
#import "SwiftBridge.h"

@interface MPNativeAd ()

@property(nonatomic) MPNativeView *associatedView;
@property(nonatomic, readonly) id<MPNativeAdAdapter> adAdapter;

- (void)willAttachToView:(UIView *)view withAdContentViews:(NSArray *)adContentViews;
- (void)adViewTapped;
- (void)nativeViewWillMoveToSuperview:(UIView *)superview;

@end

@implementation MPNativeAd (MWM)

- (void)setAdView:(UIView *)view iconView:(UIImageView *)iconImageView actionButtons:(NSArray<UIButton *> *)buttons {
  self.associatedView = (MPNativeView *)view;
  ((AdBannerView *)view).mpNativeAd = self;
  id<MPNativeAdAdapter> adapter = self.adAdapter;
  if ([adapter isKindOfClass:[FacebookNativeAdAdapter class]]) {
    FacebookNativeAdAdapter *fbAdapter = (FacebookNativeAdAdapter *)adapter;
    FBNativeBannerAd *fbNativeAd = (FBNativeBannerAd *)(fbAdapter.fbNativeAdBase);
    if (fbNativeAd) {
      [fbNativeAd unregisterView];
      [fbNativeAd registerViewForInteraction:self.associatedView
                               iconImageView:iconImageView
                              viewController:[MapViewController sharedController]
                              clickableViews:buttons];
    }
  } else {
    [self willAttachToView:self.associatedView withAdContentViews:self.associatedView.subviews];
    for (UIButton *button in buttons) {
      [button removeTarget:nil action:NULL forControlEvents:UIControlEventAllEvents];
      [button addTarget:self action:@selector(adViewTapped) forControlEvents:UIControlEventTouchUpInside];
    }
  }
}

- (void)unregister {
  id<MPNativeAdAdapter> adapter = self.adAdapter;
  self.delegate = nil;
  [self nativeViewWillMoveToSuperview:nil];
  self.associatedView = nil;
  if ([adapter isKindOfClass:[FacebookNativeAdAdapter class]]) {
    FacebookNativeAdAdapter *fbAdapter = (FacebookNativeAdAdapter *)adapter;
    [fbAdapter.fbNativeAdBase unregisterView];
  }
}

@end
