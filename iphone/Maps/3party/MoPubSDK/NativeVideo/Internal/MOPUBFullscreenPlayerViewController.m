//
//  MOPUBFullscreenPlayerViewController.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import "MOPUBFullscreenPlayerViewController.h"
#import "MOPUBPlayerView.h"
#import "MOPUBPlayerViewController.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPGlobal.h"
#import "MPNativeAdConstants.h"
#import "MOPUBActivityIndicatorView.h"
#import "UIView+MPAdditions.h"
#import "UIButton+MPAdditions.h"
#import "UIColor+MPAdditions.h"

static CGFloat const kDaaIconFullscreenLeftMargin = 16.0f;
static CGFloat const kDaaIconFullscreenTopMargin = 16.0f;
static CGFloat const kDaaIconSize = 16.0f;
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
@property (nonatomic) UIButton *daaButton;
@property (nonatomic) UIButton *closeButton;
@property (nonatomic) UIButton *ctaButton;
@property (nonatomic) MOPUBActivityIndicatorView *stallSpinner;
@property (nonatomic) UIActivityIndicatorView *playerNotReadySpinner;
@property (nonatomic) UIView *gradientView;
@property (nonatomic) CAGradientLayer *gradient;

@property (nonatomic) MOPUBPlayerViewController *playerController;
@property (nonatomic) UIView *originalParentView;
@property (nonatomic) MPAdDestinationDisplayAgent *displayAgent;
@property (nonatomic, copy) MOPUBFullScreenPlayerViewControllerDismissBlock dismissBlock;

@end

@implementation MOPUBFullscreenPlayerViewController

- (instancetype)initWithVideoPlayer:(MOPUBPlayerViewController *)playerController dismissBlock:(MOPUBFullScreenPlayerViewControllerDismissBlock)dismissBlock
{
    if (self = [super init]) {
        _playerController = playerController;
        _originalParentView = self.playerController.playerView.superview;
        _playerView = self.playerController.playerView;
        _playerController.delegate = self;
        _dismissBlock = [dismissBlock copy];
        _displayAgent = [[MPCoreInstanceProvider sharedProvider] buildMPAdDestinationDisplayAgentWithDelegate:self];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self setApplicationStatusBarHidden:YES];
    [self.playerController willEnterFullscreen];

    self.view.backgroundColor = [UIColor blackColor];
    [self.view addSubview:self.playerView];

    [self createAndAddGradientView];

    self.daaButton = [UIButton buttonWithType:UIButtonTypeCustom];
    self.daaButton.frame = CGRectMake(0, 0, kDaaIconSize, kDaaIconSize);
    [self.daaButton setImage:[UIImage imageNamed:MPResourcePathForResource(kDAAIconImageName)] forState:UIControlStateNormal];
    [self.daaButton addTarget:self action:@selector(daaButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    self.daaButton.mp_TouchAreaInsets = UIEdgeInsetsMake(kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets);
    [self.view addSubview:self.daaButton];

    self.closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [self.closeButton setImage:[UIImage imageNamed:MPResourcePathForResource(kCloseButtonImage)] forState:UIControlStateNormal];
    [self.closeButton addTarget:self action:@selector(closeButtonTapped) forControlEvents:UIControlEventTouchUpInside];
    self.closeButton.mp_TouchAreaInsets = UIEdgeInsetsMake(kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets, kDefaultButtonTouchAreaInsets);
    [self.closeButton sizeToFit];
    [self.view addSubview:self.closeButton];

    self.ctaButton = [UIButton buttonWithType:UIButtonTypeCustom];
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
    [self layoutDaaButton];
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

- (void)layoutDaaButton
{
    self.daaButton.mp_x = kDaaIconFullscreenLeftMargin;
    self.daaButton.mp_y = kDaaIconFullscreenTopMargin;
}

- (void)layoutCloseButton
{
    CGSize screenSize = MPScreenBounds().size;
    self.closeButton.mp_x = screenSize.width - kCloseButtonRightMargin - self.closeButton.mp_width;
    CGFloat daaCenterY = self.daaButton.frame.origin.y + self.daaButton.mp_height/2.0f;
    self.closeButton.mp_y = daaCenterY - self.closeButton.mp_height/2.0f;
    self.closeButton.frame = CGRectIntegral(self.closeButton.frame);
}

- (void)layoutCtaButton
{
    UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
    if (UIInterfaceOrientationIsLandscape(orientation)) {
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

- (void)daaButtonTapped
{
    [self.displayAgent displayDestinationForURL:[NSURL URLWithString:kDAAIconTapDestinationURL]];
}

#pragma mark - MOPUBPlayerViewControllerDelegate

- (void)playerPlaybackDidStart:(MOPUBPlayerViewController *)player
{
    [self removePlayerNotReadySpinner];
}

- (void)playerViewController:(MOPUBPlayerViewController *)playerViewController willShowReplayView:(MOPUBPlayerView *)view
{
    [self.view bringSubviewToFront:self.daaButton];
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

@end
