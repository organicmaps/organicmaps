//
//  MPInterstitialViewController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPInterstitialViewController.h"

#import "MPError.h"
#import "MPLogging.h"

static const CGFloat kCloseButtonPadding = 5.0;
static const CGFloat kCloseButtonEdgeInset = 5.0;
static NSString * const kCloseButtonXImageName = @"MPCloseButtonX.png";

@interface MPInterstitialViewController ()

- (void)setCloseButtonImageWithImageNamed:(NSString *)imageName;
- (void)setCloseButtonStyle:(MPInterstitialCloseButtonStyle)style;
- (void)closeButtonPressed;
- (void)dismissInterstitialAnimated:(BOOL)animated;

@end

///////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPInterstitialViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
    self.modalPresentationStyle = UIModalPresentationFullScreen;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

#pragma mark - Public

- (void)presentInterstitialFromViewController:(UIViewController *)controller complete:(void(^)(NSError *))complete
{
    if (self.presentingViewController) {
        if (complete != nil) {
            complete(NSError.fullscreenAdAlreadyOnScreen);
        }
        return;
    }

    [self willPresentInterstitial];
    [self layoutCloseButton];

    [controller presentViewController:self animated:MP_ANIMATED completion:^{
        [self didPresentInterstitial];
        if (complete != nil) {
            complete(nil);
        }
    }];
}

- (void)willPresentInterstitial
{

}

- (void)didPresentInterstitial
{

}

- (void)willDismissInterstitial
{

}

- (void)didDismissInterstitial
{

}

- (BOOL)shouldDisplayCloseButton
{
    return YES;
}

#pragma mark - Close Button

- (UIButton *)closeButton
{
    if (!_closeButton) {
        _closeButton = [MPExtendedHitBoxButton buttonWithType:UIButtonTypeCustom];
        _closeButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin |
        UIViewAutoresizingFlexibleBottomMargin;

        UIImage *closeButtonImage = [UIImage imageNamed:MPResourcePathForResource(kCloseButtonXImageName)];
        [_closeButton setImage:closeButtonImage forState:UIControlStateNormal];
        [_closeButton sizeToFit];

        [_closeButton addTarget:self
                         action:@selector(closeButtonPressed)
               forControlEvents:UIControlEventTouchUpInside];
        _closeButton.accessibilityLabel = @"Close Interstitial Ad";
    }

    return _closeButton;
}

- (void)layoutCloseButton
{
    [self.view addSubview:self.closeButton];
    CGFloat originX = self.view.bounds.size.width - kCloseButtonPadding -
    self.closeButton.bounds.size.width;
    self.closeButton.frame = CGRectMake(originX,
                                        kCloseButtonPadding,
                                        self.closeButton.bounds.size.width,
                                        self.closeButton.bounds.size.height);
    self.closeButton.touchAreaInsets = UIEdgeInsetsMake(kCloseButtonEdgeInset, kCloseButtonEdgeInset, kCloseButtonEdgeInset, kCloseButtonEdgeInset);
    [self setCloseButtonStyle:self.closeButtonStyle];
    if (@available(iOS 11, *)) {
        self.closeButton.translatesAutoresizingMaskIntoConstraints = NO;
        [NSLayoutConstraint activateConstraints:@[
                                                  [self.closeButton.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:kCloseButtonPadding],
                                                  [self.closeButton.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-kCloseButtonPadding],
                                                  ]];
    }
    [self.view bringSubviewToFront:self.closeButton];
}

- (void)setCloseButtonImageWithImageNamed:(NSString *)imageName
{
    UIImage *image = [UIImage imageNamed:imageName];
    [self.closeButton setImage:image forState:UIControlStateNormal];
    [self.closeButton sizeToFit];
}

- (void)setCloseButtonStyle:(MPInterstitialCloseButtonStyle)style
{
    _closeButtonStyle = style;
    switch (style) {
        case MPInterstitialCloseButtonStyleAlwaysVisible:
            self.closeButton.hidden = NO;
            break;
        case MPInterstitialCloseButtonStyleAlwaysHidden:
            self.closeButton.hidden = YES;
            break;
        case MPInterstitialCloseButtonStyleAdControlled:
            self.closeButton.hidden = ![self shouldDisplayCloseButton];
            break;
        default:
            self.closeButton.hidden = NO;
            break;
    }
}

- (void)closeButtonPressed
{
    [self dismissInterstitialAnimated:YES];
}

- (void)dismissInterstitialAnimated:(BOOL)animated
{
    [self willDismissInterstitial];

    UIViewController *presentingViewController = self.presentingViewController;
    // TODO: Is this check necessary?
    if (presentingViewController.presentedViewController == self) {
        [presentingViewController dismissViewControllerAnimated:MP_ANIMATED completion:^{
            [self didDismissInterstitial];
        }];
    } else {
        [self didDismissInterstitial];
    }
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    NSUInteger applicationSupportedOrientations =
    [[UIApplication sharedApplication] supportedInterfaceOrientationsForWindow:MPKeyWindow()];
    NSUInteger interstitialSupportedOrientations = applicationSupportedOrientations;
    NSString *orientationDescription = @"any";

    // Using the _orientationType, narrow down the supported interface orientations.

    if (_orientationType == MPInterstitialOrientationTypePortrait) {
        interstitialSupportedOrientations &=
        (UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown);
        orientationDescription = @"portrait";
    } else if (_orientationType == MPInterstitialOrientationTypeLandscape) {
        interstitialSupportedOrientations &= UIInterfaceOrientationMaskLandscape;
        orientationDescription = @"landscape";
    }

    // If the application does not support any of the orientations given by _orientationType,
    // just return the application's supported orientations.

    if (!interstitialSupportedOrientations) {
        MPLogInfo(@"Your application does not support this interstitial's desired orientation "
                   @"(%@).", orientationDescription);
        return applicationSupportedOrientations;
    } else {
        return interstitialSupportedOrientations;
    }
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
    NSUInteger supportedInterfaceOrientations = [self supportedInterfaceOrientations];
    UIInterfaceOrientation currentInterfaceOrientation = MPInterfaceOrientation();
    NSUInteger currentInterfaceOrientationMask = (1 << currentInterfaceOrientation);

    // First, try to display the interstitial using the current interface orientation. If the
    // current interface orientation is unsupported, just use any of the supported orientations.

    if (supportedInterfaceOrientations & currentInterfaceOrientationMask) {
        return currentInterfaceOrientation;
    } else if (supportedInterfaceOrientations & UIInterfaceOrientationMaskPortrait) {
        return UIInterfaceOrientationPortrait;
    } else if (supportedInterfaceOrientations & UIInterfaceOrientationMaskPortraitUpsideDown) {
        return UIInterfaceOrientationPortraitUpsideDown;
    } else if (supportedInterfaceOrientations & UIInterfaceOrientationMaskLandscapeLeft) {
        return UIInterfaceOrientationLandscapeLeft;
    } else {
        return UIInterfaceOrientationLandscapeRight;
    }
}

@end
