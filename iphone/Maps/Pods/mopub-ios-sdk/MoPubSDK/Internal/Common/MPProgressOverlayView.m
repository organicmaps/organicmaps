//
//  MPProgressOverlayView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <QuartzCore/QuartzCore.h>
#import "MPProgressOverlayView.h"
#import "MPGlobal.h"

// Constants
#define kProgressOverlayAlpha               0.6f
#define kProgressOverlayAnimationDuration   0.2f
#define kProgressOverlayBorderWidth         1.0f
#define kProgressOverlayCloseButtonDelay    4.0f
#define kProgressOverlayCornerRadius        8.0f
#define kProgressOverlayShadowOffset        2.0f
#define kProgressOverlayShadowOpacity       0.8f
#define kProgressOverlayShadowRadius        8.0f
#define kProgressOverlaySide                60.0f

static NSString * const kCloseButtonXImageName = @"MPCloseButtonX.png";

@interface MPProgressOverlayView ()
@property (nonatomic, strong) UIActivityIndicatorView * activityIndicator;
@property (nonatomic, strong) UIButton * closeButton;
@property (nonatomic, strong) UIView * progressContainer;
@end

@implementation MPProgressOverlayView

#pragma mark - Life Cycle

- (instancetype)initWithDelegate:(id<MPProgressOverlayViewDelegate>)delegate {
    if (self = [self initWithFrame:MPKeyWindow().bounds]) {
        self.delegate = delegate;
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        self.alpha = 0.0;
        self.opaque = NO;
        self.translatesAutoresizingMaskIntoConstraints = NO;

        // Close button.
        _closeButton = ({
            UIButton * button = [UIButton buttonWithType:UIButtonTypeCustom];
            button.alpha = 0.0;     // The close button will be animated onscreen when needed
            button.hidden = YES;    // Set to hidden to participate in autoresizing, but not capture user input

            [button addTarget:self action:@selector(closeButtonPressed) forControlEvents:UIControlEventTouchUpInside];

            UIImage * image = [UIImage imageNamed:MPResourcePathForResource(kCloseButtonXImageName)];
            [button setImage:image forState:UIControlStateNormal];

            [button sizeToFit];
            button;
        });
        [self addSubview:_closeButton];

        // Progress indicator container which provides a semi-opaque background for
        // the activity indicator to render.
        _progressContainer = ({
            CGRect frame = CGRectIntegral(CGRectMake(0, 0, kProgressOverlaySide, kProgressOverlaySide));
            UIView * container = [[UIView alloc] initWithFrame:frame];

            container.alpha = kProgressOverlayAlpha;
            container.backgroundColor = [UIColor whiteColor];
            container.opaque = NO;
            container.layer.cornerRadius = kProgressOverlayCornerRadius;
            container.layer.shadowColor = [UIColor blackColor].CGColor;
            container.layer.shadowOffset = CGSizeMake(0.0f, kProgressOverlayShadowRadius - kProgressOverlayShadowOffset);
            container.layer.shadowOpacity = kProgressOverlayShadowOpacity;
            container.layer.shadowRadius = kProgressOverlayShadowRadius;
            container.translatesAutoresizingMaskIntoConstraints = NO;

            // Container interior.
            CGFloat innerContainerSide = kProgressOverlaySide - (2.0f * kProgressOverlayBorderWidth);
            CGRect innerFrame = CGRectIntegral(CGRectMake(0, 0, innerContainerSide, innerContainerSide));
            UIView * innerContainer = [[UIView alloc] initWithFrame:innerFrame];
            innerContainer.backgroundColor = [UIColor blackColor];
            innerContainer.layer.cornerRadius = kProgressOverlayCornerRadius - kProgressOverlayBorderWidth;
            innerContainer.opaque = NO;
            innerContainer.translatesAutoresizingMaskIntoConstraints = NO;

            // Center the interior container
            [container addSubview:innerContainer];
            innerContainer.center = container.center;

            container;
        });
        [self addSubview:_progressContainer];

        // Progress indicator.
        _activityIndicator = ({
            UIActivityIndicatorView * indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
            [indicator sizeToFit];
            [indicator startAnimating];
            indicator;
        });
        [self addSubview:_activityIndicator];

        // Needs initial layout
        [self setNeedsLayout];

        // Listen for device rotation notifications
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(deviceOrientationDidChange:) name:UIDeviceOrientationDidChangeNotification object:nil];
    }
    return self;
}

- (void)dealloc {
    // Unregister self from notifications
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Layout

- (void)layoutSubviews {
    [super layoutSubviews];

    // This layout is always full screen for the Application frame inclusive
    // of the safe area.
    CGRect appFrame = MPApplicationFrame(YES);

    // Update the size of this view to always match that of the key window.
    self.frame = MPKeyWindow().bounds;

    // Close button should be in the upper right corner.
    self.closeButton.frame = CGRectMake(appFrame.origin.x + appFrame.size.width - self.closeButton.frame.size.width,
                                        appFrame.origin.y,
                                        self.closeButton.frame.size.width,
                                        self.closeButton.frame.size.height);

    // Progress indicator container should be centered.
    self.progressContainer.center = self.center;

    // Progress indicator should be centered.
    self.activityIndicator.center = self.center;
}

#pragma mark - Public Methods

- (void)show {
    // Add self to the key window
    UIWindow * keyWindow = MPKeyWindow();
    [keyWindow addSubview:self];

    // Re-layout needed.
    [self setNeedsLayout];

    // Animate self on screen
    [UIView animateWithDuration:kProgressOverlayAnimationDuration animations:^{
        self.alpha = 1.0;
    } completion:^(BOOL finished) {
        if ([self.delegate respondsToSelector:@selector(overlayDidAppear)]) {
            [self.delegate overlayDidAppear];
        }
    }];

    // Show the close button after kProgressOverlayCloseButtonDelay delay to allow
    // the user an out if progress gets stuck.
    [self performSelector:@selector(enableCloseButton) withObject:nil afterDelay:kProgressOverlayCloseButtonDelay];
}

- (void)hide {
    // Cancel any pending enabling of the close button
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(enableCloseButton) object:nil];

    // Hide the close button and make it non-user interactable immediately
    self.closeButton.alpha = 0.0f;
    self.closeButton.hidden = YES;

    // Animate removal of self from the key window
    [UIView animateWithDuration:kProgressOverlayAnimationDuration animations:^{
        self.alpha = 0.0;
    } completion:^(BOOL finished) {
        [self removeFromSuperview];
    }];
}

#pragma mark - Notification Handlers

- (void)deviceOrientationDidChange:(NSNotification *)notification {
    [self setNeedsLayout];
}

#pragma mark - Close Button

- (void)enableCloseButton {
    self.closeButton.hidden = NO;
    [UIView animateWithDuration:kProgressOverlayAnimationDuration animations:^{
        self.closeButton.alpha = 1.0f;
    }];
}

- (void)closeButtonPressed {
    if ([self.delegate respondsToSelector:@selector(overlayCancelButtonPressed)]) {
        [self.delegate overlayCancelButtonPressed];
    }
}

@end
