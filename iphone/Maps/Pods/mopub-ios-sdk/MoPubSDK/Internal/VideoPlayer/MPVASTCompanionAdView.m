//
//  MPVASTCompanionAdView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPImageLoader.h"
#import "MPLogging.h"
#import "MPVASTCompanionAdView.h"
#import "MRController.h"
#import "UIView+MPAdditions.h"

@interface MPVASTCompanionAdView ()

@property (nonatomic, strong) MPVASTCompanionAd *ad;
@property (nonatomic, strong) MRController *mraidController;
@property (nonatomic, strong) UIImageView *imageView;
@property (nonatomic, strong) MPImageLoader *imageLoader;
@property (nonatomic, strong) UITapGestureRecognizer *tapGestureRecognizer; // exclusively for image resource
@property (nonatomic, assign, readwrite) BOOL isWebContent;

@end

@interface MPVASTCompanionAdView (UIGestureRecognizerDelegate) <UIGestureRecognizerDelegate>
@end

@interface MPVASTCompanionAdView (MPImageLoaderDelegate) <MPImageLoaderDelegate>
@end

@interface MPVASTCompanionAdView (MRControllerDelegate) <MRControllerDelegate>
@end

@implementation MPVASTCompanionAdView

- (instancetype)initWithCompanionAd:(MPVASTCompanionAd *)ad {
    self = [super init];
    if (self) {
        _ad = ad;
        _isWebContent = ad.resourceToDisplay.type != MPVASTResourceType_StaticImage;
        self.accessibilityLabel = @"Companion Ad";
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];

    // Ensure that web content ads should occupy the entire container whenever the layout changes.
    if (self.isWebContent) {
        [self.subviews enumerateObjectsUsingBlock:^(__kindof UIView * _Nonnull subview, NSUInteger idx, BOOL * _Nonnull stop) {
            subview.frame = self.bounds;
        }];
    }
}

- (void)loadCompanionAd {
    MPVASTResource *resource = self.ad.resourceToDisplay;
    if (resource.type == MPVASTResourceType_StaticImage) {
        [self loadStaticImageResource:resource];
    }
    else {
        [self loadWebResource:resource];
    }
}

- (void)loadStaticImageResource:(MPVASTResource *)resource {
    if (self.imageView != nil) {
        return; // already loaded
    }

    NSURL *imageURL = [NSURL URLWithString:resource.content];
    if (imageURL == nil) {
        [self.delegate companionAdView:self didTriggerEvent:MPVASTResourceViewEvent_FailedToLoadView];
        return; // invalid image URL
    }

    // the ad creative is responsible for handling user interaction for other resource types
    self.tapGestureRecognizer = [[UITapGestureRecognizer alloc]
                                 initWithTarget:self
                                 action:@selector(handleClickThrough)];
    self.tapGestureRecognizer.delegate = self;
    [self addGestureRecognizer:self.tapGestureRecognizer];

    // create and layout the image view
    self.imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
    self.imageView.contentMode = UIViewContentModeScaleAspectFit;
    [self addSubview:self.imageView];
    self.imageView.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [self.imageView.mp_safeTopAnchor constraintEqualToAnchor:self.mp_safeTopAnchor],
        [self.imageView.mp_safeLeadingAnchor constraintEqualToAnchor:self.mp_safeLeadingAnchor],
        [self.imageView.mp_safeBottomAnchor constraintEqualToAnchor:self.mp_safeBottomAnchor],
        [self.imageView.mp_safeTrailingAnchor constraintEqualToAnchor:self.mp_safeTrailingAnchor]
    ]];

    // load the image
    self.imageLoader = [MPImageLoader new];
    self.imageLoader.delegate = self;
    [self.imageLoader loadImageForURL:imageURL intoImageView:self.imageView];
}

- (void)loadWebResource:(MPVASTResource *)resource {
    if (self.mraidController != nil) {
        return; // already loaded
    }

    self.mraidController = [[MRController alloc] initWithAdViewFrame:self.ad.safeAdViewBounds
                                               supportedOrientations:MPInterstitialOrientationTypeAll // companion ad does not inherit `x-orientation` from the main ad
                                                     adPlacementType:MRAdViewPlacementTypeInline
                                                            delegate:self];
    self.mraidController.delegate = self;
    [self.mraidController disableClickthroughWebBrowser]; // let the companion ad view delegate handle click-through instead of the `MRController`

    // Resource content is a URL. Prioritize loading the URL.
    NSURL *resourceUrl = [NSURL URLWithString:resource.content];
    if (resourceUrl != nil) {
        [self.mraidController loadVASTCompanionAdUrl:resourceUrl];
    }
    // Resource content should be an HTML snippet or full HTML doc.
    else {
        NSString *html = [MPVASTResource fullHTMLRespresentationForContent:resource.content
                                                                      type:resource.type
                                                             containerSize:self.ad.safeAdViewBounds.size];
        [self.mraidController loadVASTCompanionAd:html];
    }
}

- (void)handleClickThrough {
    [self.delegate companionAdView:self didTriggerEvent:MPVASTResourceViewEvent_ClickThrough];
}

@end

#pragma mark - UIGestureRecognizerDelegate

@implementation MPVASTCompanionAdView (UIGestureRecognizerDelegate)

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES; // need this for the web view to recognize the tap
}

@end

#pragma mark - MPImageLoaderDelegate

@implementation MPVASTCompanionAdView (MPImageLoaderDelegate)

- (BOOL)nativeAdViewInViewHierarchy {
    // Always return YES because the companion ad is not recycled like a table cell. The companion ad
    // view only exists while it's still in the view hierarchy.
    return YES;
}

- (void)imageLoader:(MPImageLoader *)imageLoaded didLoadImageIntoImageView:(UIImageView *)imageView {
    if (self.imageView != imageView) {
        return;
    }

    _isLoaded = YES;
    [self.delegate companionAdView:self didTriggerEvent:MPVASTResourceViewEvent_DidLoadView];
}

@end

#pragma mark - MRControllerDelegate

@implementation MPVASTCompanionAdView (MRControllerDelegate)

- (UIViewController *)viewControllerForPresentingModalView {
    return self.delegate.viewControllerForPresentingModalMRAIDExpandedView;
}

- (void)appShouldSuspendForAd:(UIView *)adView {
    // No op. It's the owner's (such as custom event) responsibility for handling app status changes
}

- (void)appShouldResumeFromAd:(UIView *)adView {
    // No op. It's the owner's (such as custom event) responsibility for handling app status changes
}

- (void)adDidLoad:(UIView *)adView {
    /*
     Note: Do not apply layout constrains on the ad view, otherwise the layout engine mighe be confused.

     Current the MRAID two-part expandable ad view container does not work well with layout constraints
     while the ad view is being transfer between different parents. Since companion ad view container
     always has a fixed size, we can safely use the good old `bounds` for laying out the ad view.
     */

    // Web content ads should occupy the entire container.
    adView.frame = self.bounds;
    [self addSubview:adView];
    [self layoutIfNeeded];

    _isLoaded = YES; // keep this before delegate callback
    [self.delegate companionAdView:self didTriggerEvent:MPVASTResourceViewEvent_DidLoadView];
}

- (void)adDidFailToLoad:(UIView *)adView {
    [self.delegate companionAdView:self didTriggerEvent:MPVASTResourceViewEvent_FailedToLoadView];
}

- (void)adWillClose:(UIView *)adView {
    [self.delegate companionAdViewRequestDismiss:self];
}

- (void)adDidClose:(UIView *)adView {
    // No op. Let `adWillClose:` forward the dismiss request to the delegate
}

- (void)adDidReceiveClickthrough:(NSURL *)url {
    [self.delegate companionAdView:self didTriggerOverridingClickThrough:url];
}

- (void)adWillExpand:(UIView *)adView {
    // No op. Unlike regular inline ads, companion ad is not refreshed automatically and thus no need to pause refresh timer.
}

- (void)adDidCollapse:(UIView *)adView {
    // No op. Unlike regular inline ads, companion ad is not refreshed automatically and thus no need to resume refresh timer.
}

@end
