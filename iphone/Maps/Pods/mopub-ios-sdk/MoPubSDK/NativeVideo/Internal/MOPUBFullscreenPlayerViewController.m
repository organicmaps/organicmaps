//
//  MOPUBFullscreenPlayerViewController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <AVFoundation/AVFoundation.h>
#import "MOPUBFullscreenPlayerViewController.h"
#import "MOPUBPlayerView.h"
#import "MOPUBPlayerViewController.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPExtendedHitBoxButton.h"
#import "MPHTTPNetworkSession.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MPMemoryCache.h"
#import "MPNativeAdConstants.h"
#import "MPURLRequest.h"
#import "MOPUBActivityIndicatorView.h"
#import "UIView+MPAdditions.h"
#import "UIColor+MPAdditions.h"

static CGFloat const kPrivacyIconFullscreenLeftMargin = 16.0f;
static CGFloat const kPrivacyIconFullscreenTopMargin = 16.0f;
static CGFloat const kPrivacyIconSize = 16.0f;
static CGFloat const kCloseButtonRightMargin = 16.0f;
static CGFloat const kDefaultButtonTouchAreaInsets = 10.0f;

static NSString * const kCloseButtonImage = @"MPCloseBtn.png";
static NSString * const kCtaButtonTitleText = @"Learn More";
static CGFloat const kCtaButtonTopMarginPortrait = 15.0f;
static CGFloat const kCtaButtonTrailingMarginLandscape = 15.0f;
static CGFloat const kCtaButtonBottomMarginLandscape = 15.0f;
static CGFloat const kCtaButtonBottomCornerRadius = 4.0f;
static CGFloat const kCtaButtonBottomBorderWidth = 0.5f;
static CGFloat const kCtaButtonBottomFontSize = 18.0f;
static CGFloat const kCtaButtonContentInsetsHorizontal = 35.0f;
static CGFloat const kCtaButtonContentInsetsVertical = 10.0f;
static CGFloat const kCtaButtonBackgroundAlpha = 0.2f;
static NSString * const kCtaButtonBackgroundColor = @"#000000";

static NSString * const kTopGradientColor = @"#000000";
static NSString * const kBottomGradientColor= @"#000000";
static CGFloat const kTopGradientAlpha = 0.4f;
static CGFloat const kBottomGradientAlpha = 0.0f;
static CGFloat const kGradientHeight = 42;

static CGFloat const kStallSpinnerSize = 35.0f;

@interface MOPUBFullscreenPlayerViewController () <MPAdDestinationDisplayAgentDelegate, MOPUBPlayerViewControllerDelegate>

// UI components
@property (nonatomic, strong) MPExtendedHitBoxButton *privacyButton;
@property (nonatomic, strong) MPExtendedHitBoxButton *closeButton;
@property (nonatomic, strong) MPExtendedHitBoxButton *ctaButton;
@property (nonatomic) MOPUBActivityIndicatorView *stallSpinner;
@property (nonatomic) UIActivityIndicatorView *playerNotReadySpinner;
@property (nonatomic) UIView *gradientView;
@property (nonatomic) CAGradientLayer *gradient;

@property (nonatomic) MOPUBPlayerViewController *playerController;
@property (nonatomic) UIView *originalParentView;
@property (nonatomic) id<MPAdDestinationDisplayAgent> displayAgent;
@property (nonatomic, copy) MOPUBFullScreenPlayerViewControllerDismissBlock dismissBlock;

// Overrides
@property (nonatomic, copy) NSString * overridePrivacyIcon;
@property (nonatomic, strong) UIImage * overridePrivacyIconImage;
@property (nonatomic, copy) NSString * overridePrivacyClickUrl;

@end

@implementation MOPUBFullscreenPlayerViewController

- (instancetype)initWithVideoPlayer:(MOPUBPlayerViewController *)playerController
                 nativeAdProperties:(NSDictionary *)properties
                       dismissBlock:(MOPUBFullScreenPlayerViewControllerDismissBlock)dismissBlock
{
    if (self = [super init]) {
        _playerController = playerController;
        _originalParentView = self.playerController.playerView.superview;
        _playerView = self.playerController.playerView;
        _playerController.delegate = self;
        _dismissBlock = [dismissBlock copy];
        _displayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];
        _overridePrivacyIcon = properties[kAdPrivacyIconImageUrlKey];
        _overridePrivacyIconImage = properties[kAdPrivacyIconUIImageKey];
        _overridePrivacyClickUrl = properties[kAdPrivacyIconClickUrlKey];
        self.modalPresentationStyle = UIModalPresentationFullScreen;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self.playerController willEnterFullscreen];

    self.view.backgroundColor = [UIColor blackColor];
    [self.view addSubview:self.playerView];

    [self createAndAddGradientView];

    self.privacyButton = [MPExtendedHitBoxButton buttonWithType:UIButtonTypeCustom];
    self.privacyButton.frame = CGRectMake(0, 0, kPrivacyIconSize, kPrivacyIconSize);
    [self setPrivacyIconImageForButton:self.privacyButton];
    [self.privacyButton addTarget:self action:@selector(privacyButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    self.privacyButton.touchAreaInsets = UIEdgeInsetsMake(kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets);
    [self.view addSubview:self.privacyButton];

    self.closeButton = [MPExtendedHitBoxButton buttonWithType:UIButtonTypeCustom];
    [self.closeButton setImage:[UIImage imageNamed:MPResourcePathForResource(kCloseButtonImage)] forState:UIControlStateNormal];
    [self.closeButton addTarget:self action:@selector(closeButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    self.closeButton.touchAreaInsets = UIEdgeInsetsMake(kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets);
    [self.closeButton sizeToFit];
    [self.view addSubview:self.closeButton];

    self.ctaButton = [MPExtendedHitBoxButton buttonWithType:UIButtonTypeCustom];
    [self.ctaButton setTitle:kCtaButtonTitleText forState:UIControlStateNormal];
    [self.ctaButton setBackgroundColor:[UIColor mp_colorFromHexString:kCtaButtonBackgroundColor alpha:kCtaButtonBackgroundAlpha]];
    [self.ctaButton addTarget:self action:@selector(ctaButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    self.ctaButton.layer.cornerRadius = kCtaButtonBottomCornerRadius;
    self.ctaButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:kCtaButtonBottomFontSize];
    [self.ctaButton setContentEdgeInsets:UIEdgeInsetsMake(kCtaButtonContentInsetsVertical, kCtaButtonContentInsetsHorizontal, kCtaButtonContentInsetsVertical, kCtaButtonContentInsetsHorizontal)];
    [[self.ctaButton layer] setBorderWidth:kCtaButtonBottomBorderWidth];
    [[self.ctaButton layer] setBorderColor:[UIColor whiteColor].CGColor];
    [self.ctaButton sizeToFit];
    [self.view addSubview:self.ctaButton];

    if (!self.playerController.isReadyToPlay) {
        [self createPlayerNotReadySpinner];
    }

    // Once the video enters fullscreen mode, we should resume the playback if it is paused.
    if (self.playerController.paused) {
        [self.playerController resume];
    }
}

- (void)setPrivacyIconImageForButton:(UIButton *)button
{
    if (button == nil) {
        return;
    }

    // A cached privacy information icon image exists; it should be used.
    if (self.overridePrivacyIconImage != nil) {
        [button setImage:self.overridePrivacyIconImage forState:UIControlStateNormal];
    }
    // No cached privacy information icon image was cached, but there is a URL for the
    // icon. Go fetch the icon and populate the UIImageView when complete.
    else if (self.overridePrivacyIcon != nil) {
        NSURL *iconUrl = [NSURL URLWithString:self.overridePrivacyIcon];
        MPURLRequest *imageRequest = [MPURLRequest requestWithURL:iconUrl];

        [MPHTTPNetworkSession startTaskWithHttpRequest:imageRequest responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
            // Cache the successfully retrieved icon image
            [MPMemoryCache.sharedInstance setData:data forKey:self.overridePrivacyIcon];

            // Populate the button
            [button setImage:[UIImage imageWithData:data] forState:UIControlStateNormal];
        } errorHandler:^(NSError * _Nonnull error) {
            MPLogInfo(@"Failed to retrieve privacy icon from %@", self.overridePrivacyIcon);
        }];
    }
    // Default to built in MoPub privacy icon.
    else {
        [button setImage:[UIImage imageNamed:MPResourcePathForResource(kPrivacyIconImageName)] forState:UIControlStateNormal];
    }
}

- (void)createAndAddGradientView
{
    // Create the gradient
    self.gradientView = [UIView new];
    self.gradientView.userInteractionEnabled = NO;
    UIColor *topColor = [UIColor mp_colorFromHexString:kTopGradientColor alpha:kTopGradientAlpha];
    UIColor *bottomColor= [UIColor mp_colorFromHexString:kBottomGradientColor alpha:kBottomGradientAlpha];
    self.gradient = [CAGradientLayer layer];
    self.gradient.colors = [NSArray arrayWithObjects: (id)topColor.CGColor, (id)bottomColor.CGColor, nil];
    CGSize screenSize = MPScreenBounds().size;
    self.gradient.frame = CGRectMake(0, 0, screenSize.width, kGradientHeight);

    //Add gradient to view
    [self.gradientView.layer insertSublayer:self.gradient atIndex:0];
    [self.view addSubview:self.gradientView];
}

- (void)showStallSpinner
{
    if (!self.stallSpinner) {
        self.stallSpinner = [[MOPUBActivityIndicatorView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, kStallSpinnerSize, kStallSpinnerSize)];
        [self.view addSubview:self.stallSpinner];
    }
}

- (void)hideStallSpinner
{
    if (self.stallSpinner) {
        [self.stallSpinner stopAnimating];
        [self.stallSpinner removeFromSuperview];
    }
}

- (void)createPlayerNotReadySpinner
{
    if (!self.playerNotReadySpinner) {
        self.playerNotReadySpinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
        [self.view addSubview:self.playerNotReadySpinner];
        [self.playerNotReadySpinner startAnimating];
    }
}

- (void)removePlayerNotReadySpinner
{
    [self.playerNotReadySpinner stopAnimating];
    [self.playerNotReadySpinner removeFromSuperview];
    self.playerNotReadySpinner = nil;
}

#pragma mark - Layout UI components

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];

    [self layoutPlayerView];
    [self layoutPrivacyButton];
    [self layoutCloseButton];
    [self layoutCtaButton];
    [self layoutStallSpinner];
    [self layoutPlayerNotReadySpinner];
    [self layoutGradientView];
}

- (void)layoutPlayerView
{
    CGSize screenSize = MPScreenBounds().size;
    self.playerView.videoGravity = AVLayerVideoGravityResizeAspectFill;
    if (UIInterfaceOrientationIsLandscape(MPInterfaceOrientation())) {
        self.playerView.frame = CGRectMake(0, 0, screenSize.width, screenSize.height);
    } else {
        self.playerView.mp_width = screenSize.width;
        self.playerView.mp_height = self.playerView.mp_width/self.playerController.videoAspectRatio;
        self.playerView.center = self.view.center;
        self.playerView.frame = CGRectIntegral(self.playerView.frame);
    }
}

- (void)layoutPrivacyButton
{
    self.privacyButton.mp_x = kPrivacyIconFullscreenLeftMargin;
    self.privacyButton.mp_y = kPrivacyIconFullscreenTopMargin;
}

- (void)layoutCloseButton
{
    CGSize screenSize = MPScreenBounds().size;
    self.closeButton.mp_x = screenSize.width - kCloseButtonRightMargin - self.closeButton.mp_width;
    CGFloat privacyCenterY = self.privacyButton.frame.origin.y + self.privacyButton.mp_height/2.0f;
    self.closeButton.mp_y = privacyCenterY - self.closeButton.mp_height/2.0f;
    self.closeButton.frame = CGRectIntegral(self.closeButton.frame);
}

- (void)layoutCtaButton
{
    CGRect applicationFrame = MPApplicationFrame(YES);
    BOOL isLandscapeOrientation = applicationFrame.size.width > applicationFrame.size.height;

    if (isLandscapeOrientation) {
        self.ctaButton.mp_x = CGRectGetMaxX(self.playerView.frame) - kCtaButtonTrailingMarginLandscape - CGRectGetWidth(self.ctaButton.bounds);
        self.ctaButton.mp_y = CGRectGetMaxY(self.playerView.frame) - kCtaButtonBottomMarginLandscape - CGRectGetHeight(self.ctaButton.bounds);
    } else {
        self.ctaButton.center = self.view.center;
        self.ctaButton.mp_y = CGRectGetMaxY(self.playerView.frame) + kCtaButtonTopMarginPortrait;
        self.ctaButton.frame = CGRectIntegral(self.ctaButton.frame);
    }
}

- (void)layoutStallSpinner
{
    if (self.stallSpinner) {
        CGSize screenSize = MPScreenBounds().size;
        self.stallSpinner.center = CGPointMake(screenSize.width/2.0f, screenSize.height/2.0f);
        self.stallSpinner.frame = CGRectIntegral(self.stallSpinner.frame);
    }
}

- (void)layoutPlayerNotReadySpinner
{
    if (self.playerNotReadySpinner) {
        CGSize screenSize = MPScreenBounds().size;
        self.playerNotReadySpinner.center = CGPointMake(screenSize.width/2.0f, screenSize.height/2.0f);
        self.playerNotReadySpinner.frame = CGRectIntegral(self.playerNotReadySpinner.frame);
    }
}

- (void)layoutGradientView
{
    if (UIInterfaceOrientationIsLandscape(MPInterfaceOrientation())) {
        self.gradientView.hidden = NO;
    } else {
        self.gradientView.hidden = YES;
    }
    CGSize screenSize = MPScreenBounds().size;
    self.gradient.frame = CGRectMake(0, 0, screenSize.width, kGradientHeight);
    self.gradientView.frame = CGRectMake(0, 0, screenSize.width, kGradientHeight);
}

#pragma mark - button tap

- (void)closeButtonTapped
{
    [self dismissViewControllerAnimated:NO completion:^{
        if (self.dismissBlock) {
            self.dismissBlock(self.originalParentView);
        }
    }];
}

- (void)ctaButtonTapped
{
    if ([self.delegate respondsToSelector:@selector(ctaTapped:)]) {
        [self.delegate ctaTapped:self];
    }
    [self.displayAgent displayDestinationForURL:self.playerController.defaultActionURL];
}

- (void)privacyButtonTapped
{
    NSURL *defaultPrivacyClickUrl = [NSURL URLWithString:kPrivacyIconTapDestinationURL];
    NSURL *overridePrivacyClickUrl = ({
        NSString *url = self.overridePrivacyClickUrl;
        (url != nil ? [NSURL URLWithString:url] : nil);
    });

    [self.displayAgent displayDestinationForURL:(overridePrivacyClickUrl != nil ? overridePrivacyClickUrl : defaultPrivacyClickUrl)];
}

#pragma mark - MOPUBPlayerViewControllerDelegate

- (void)playerPlaybackDidStart:(MOPUBPlayerViewController *)player
{
    [self removePlayerNotReadySpinner];
}

- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController willShowReplayView:(MOPUBPlayerView *)view
{
    [self.view bringSubviewToFront:self.privacyButton];
}

- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController didStall:(MOPUBAVPlayer *)player
{
    if (self.stallSpinner) {
        if (!self.stallSpinner.superview) {
            [self.view addSubview:self.stallSpinner];
        }
        if (!self.stallSpinner.isAnimating) {
            [self.stallSpinner startAnimating];
        }
    } else {
        [self showStallSpinner];
        [self.stallSpinner startAnimating];
    }
}

- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController didRecoverFromStall:(MOPUBAVPlayer *)player
{
    [self hideStallSpinner];
}

- (void)playerDidProgressToTime:(NSTimeInterval)playbackTime
{
    if ([self.delegate respondsToSelector:@selector(playerDidProgressToTime:)]) {
        [self.delegate playerDidProgressToTime:playbackTime];
    }
}

#pragma mark - <MPAdDestinationDisplayAgent>

- (UIViewController *)viewControllerForPresentingModalView
{
    return self;
}

- (void)displayAgentWillPresentModal
{
    [self.playerController pause];
}

- (void)displayAgentWillLeaveApplication
{
    [self.playerController pause];
    [self.delegate fullscreenPlayerWillLeaveApplication:self];
}

- (void)displayAgentDidDismissModal
{
    [self.playerController resume];
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

@end
