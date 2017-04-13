//
//  MPInterstitialViewController.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPInterstitialViewController.h"

#import "MPGlobal.h"
#import "MPLogging.h"
#import "UIButton+MPAdditions.h"

static const CGFloat kCloseButtonPadding = 5.0;
static const CGFloat kCloseButtonEdgeInset = 5.0;
static NSString * const kCloseButtonXImageName = @"MPCloseButtonX.png";

@interface MPInterstitialViewController ()

@property (nonatomic, assign) BOOL applicationHasStatusBar;

- (void)setCloseButtonImageWithImageNamed:(NSString *)imageName;
- (void)setCloseButtonStyle:(MPInterstitialCloseButtonStyle)style;
- (void)closeButtonPressed;
- (void)dismissInterstitialAnimated:(BOOL)animated;
- (void)setApplicationStatusBarHidden:(BOOL)hidden;

@end

///////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPInterstitialViewController

@synthesize closeButton = _closeButton;
@synthesize closeButtonStyle = _closeButtonStyle;
@synthesize orientationType = _orientationType;
@synthesize applicationHasStatusBar = _applicationHasStatusBar;
@synthesize delegate = _delegate;


- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
}

#pragma mark - Public

- (void)presentInterstitialFromViewController:(UIViewController *)controller
{
    if (self.presentingViewController) {
        MPLogWarn(@"Cannot present an interstitial that is already on-screen.");
        return;
    }

    [self willPresentInterstitial];

    self.applicationHasStatusBar = !([UIApplication sharedApplication].isStatusBarHidden);
    [self setApplicationStatusBarHidden:YES];

    [self layoutCloseButton];

    [controller presentViewController:self animated:MP_ANIMATED completion:^{
        [self didPresentInterstitial];
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
        _closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
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
    CGFloat originX = self.view.bounds.size.width - kCloseButtonPadding -
    self.closeButton.bounds.size.width;
    self.closeButton.frame = CGRectMake(originX,
                                        kCloseButtonPadding,
                                        self.closeButton.bounds.size.width,
                                        self.closeButton.bounds.size.height);
    self.closeButton.mp_TouchAreaInsets = UIEdgeInsetsMake(kCloseButtonEdgeInset, kCloseButtonEdgeInset, kCloseButtonEdgeInset, kCloseButtonEdgeInset);
    [self setCloseButtonStyle:self.closeButtonStyle];
    [self.view addSubview:self.closeButton];
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
    [self setApplicationStatusBarHidden:!self.applicationHasStatusBar];

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

#pragma mark - Hidding status bar (pre-iOS 7)

- (void)setApplicationStatusBarHidden:(BOOL)hidden
{
    [[UIApplication sharedApplication] mp_preIOS7setApplicationStatusBarHidden:hidden];
}

#pragma mark - Hidding status bar (iOS 7 and above)

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

#pragma mark - Autorotation (iOS 6.0 and above)

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
- (BOOL)shouldAutorotate
{
    return YES;
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_9_0
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
#else
- (NSUInteger)supportedInterfaceOrientations
#endif
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
        MPLogError(@"Your application does not support this interstitial's desired orientation "
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
#endif

#pragma mark - Autorotation (before iOS 6.0)

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if (_orientationType == MPInterstitialOrientationTypePortrait) {
        return (interfaceOrientation == UIInterfaceOrientationPortrait ||
                interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown);
    } else if (_orientationType == MPInterstitialOrientationTypeLandscape) {
        return (interfaceOrientation == UIInterfaceOrientationLandscapeLeft ||
                interfaceOrientation == UIInterfaceOrientationLandscapeRight);
    } else {
        return YES;
    }
}

@end
