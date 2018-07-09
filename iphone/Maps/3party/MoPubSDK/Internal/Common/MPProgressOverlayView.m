//
//  MPProgressOverlayView.m
//  MoPub
//
//  Created by Andrew He on 7/18/12.
//  Copyright 2012 MoPub, Inc. All rights reserved.
//

#import "MPProgressOverlayView.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import <QuartzCore/QuartzCore.h>

static NSString * const kCloseButtonXImageName = @"MPCloseButtonX.png";

@interface MPProgressOverlayView ()

- (void)updateCloseButtonPosition;
- (void)registerForDeviceOrientationNotifications;
- (void)unregisterForDeviceOrientationNotifications;
- (void)deviceOrientationDidChange:(NSNotification *)notification;
- (void)setTransformForCurrentOrientationAnimated:(BOOL)animated;
- (void)setTransformForAllSubviews:(CGAffineTransform)transform;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

#define kProgressOverlaySide                60.0
#define kProgressOverlayBorderWidth         1.0
#define kProgressOverlayCornerRadius        8.0
#define kProgressOverlayShadowOpacity       0.8
#define kProgressOverlayShadowRadius        8.0
#define kProgressOverlayCloseButtonDelay    4.0

static void exponentialDecayInterpolation(void *info, const CGFloat *input, CGFloat *output);

@implementation MPProgressOverlayView

@synthesize delegate = _delegate;
@synthesize closeButton = _closeButton;

- (id)initWithDelegate:(id<MPProgressOverlayViewDelegate>)delegate
{
    self = [self initWithFrame:MPKeyWindow().bounds];
    if (self) {
        self.delegate = delegate;
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.alpha = 0.0;
        self.opaque = NO;

        // Close button.
        _closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
        _closeButton.alpha = 0.0;
        _closeButton.hidden = YES;
        [_closeButton addTarget:self
                         action:@selector(closeButtonPressed)
               forControlEvents:UIControlEventTouchUpInside];
        UIImage *image = [UIImage imageNamed:MPResourcePathForResource(kCloseButtonXImageName)];
        [_closeButton setImage:image forState:UIControlStateNormal];
        [_closeButton sizeToFit];

        _closeButtonPortraitCenter =
            CGPointMake(self.bounds.size.width - 6.0 - CGRectGetMidX(_closeButton.bounds),
                        6.0 + CGRectGetMidY(_closeButton.bounds));

        _closeButton.center = _closeButtonPortraitCenter;
        [self addSubview:_closeButton];

        // Progress indicator container.
        CGRect outerFrame = CGRectMake(0, 0, kProgressOverlaySide, kProgressOverlaySide);
        _outerContainer = [[UIView alloc] initWithFrame:outerFrame];
        _outerContainer.alpha = 0.6;
        _outerContainer.backgroundColor = [UIColor whiteColor];
        _outerContainer.center = self.center;
        _outerContainer.frame = CGRectIntegral(_outerContainer.frame);
        _outerContainer.opaque = NO;
        _outerContainer.layer.cornerRadius = kProgressOverlayCornerRadius;
        if ([_outerContainer.layer respondsToSelector:@selector(setShadowColor:)]) {
            _outerContainer.layer.shadowColor = [UIColor blackColor].CGColor;
            _outerContainer.layer.shadowOffset = CGSizeMake(0.0f, kProgressOverlayShadowRadius - 2.0f);
            _outerContainer.layer.shadowOpacity = kProgressOverlayShadowOpacity;
            _outerContainer.layer.shadowRadius = kProgressOverlayShadowRadius;
        }
        [self addSubview:_outerContainer];

        CGFloat innerSide = kProgressOverlaySide - 2 * kProgressOverlayBorderWidth;
        CGRect innerFrame = CGRectMake(0, 0, innerSide, innerSide);
        _innerContainer = [[UIView alloc] initWithFrame:innerFrame];
        _innerContainer.backgroundColor = [UIColor blackColor];
        _innerContainer.center = CGPointMake(CGRectGetMidX(_outerContainer.bounds),
                                             CGRectGetMidY(_outerContainer.bounds));
        _innerContainer.frame = CGRectIntegral(_innerContainer.frame);
        _innerContainer.layer.cornerRadius =
            kProgressOverlayCornerRadius - kProgressOverlayBorderWidth;
        _innerContainer.opaque = NO;
        [_outerContainer addSubview:_innerContainer];

        // Progress indicator.

        _activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:
                              UIActivityIndicatorViewStyleWhiteLarge];
        [_activityIndicator sizeToFit];
        [_activityIndicator startAnimating];
        _activityIndicator.center = self.center;
        _activityIndicator.frame = CGRectIntegral(_activityIndicator.frame);
        [self addSubview:_activityIndicator];

        [self registerForDeviceOrientationNotifications];
    }
    return self;
}

- (void)dealloc
{
    [self unregisterForDeviceOrientationNotifications];
}

#pragma mark - Public Methods

- (void)show
{
    [MPKeyWindow() addSubview:self];

    [self setTransformForCurrentOrientationAnimated:NO];

    if (MP_ANIMATED) {
        [UIView animateWithDuration:0.2 animations:^{
            self.alpha = 1.0;
        } completion:^(BOOL finished) {
            if ([self.delegate respondsToSelector:@selector(overlayDidAppear)]) {
                [self.delegate overlayDidAppear];
            }
        }];
    } else {
        self.alpha = 1.0;
        if ([self.delegate respondsToSelector:@selector(overlayDidAppear)]) {
            [self.delegate overlayDidAppear];
        }
    }

    [self performSelector:@selector(enableCloseButton)
               withObject:nil
               afterDelay:kProgressOverlayCloseButtonDelay];
}

- (void)hide
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(enableCloseButton) object:nil];

    self.closeButton.hidden = YES;
    self.closeButton.alpha = 0.0f;

    if (MP_ANIMATED) {
        [UIView animateWithDuration:0.2 animations:^{
            self.alpha = 0.0;
        } completion:^(BOOL finished) {
            [self removeFromSuperview];
        }];
    } else {
        self.alpha = 0.0;
        [self removeFromSuperview];
    }
}

#pragma mark - Drawing and Layout

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

    static const CGFloat input_value_range[2] = {0, 1};
    static const CGFloat output_value_range[8] = {0, 1, 0, 1, 0, 1, 0, 1};
    CGFunctionCallbacks callbacks = {0, exponentialDecayInterpolation, NULL};

    CGFunctionRef shadingFunction = CGFunctionCreate((__bridge void *)(self), 1, input_value_range, 4,
                                                     output_value_range, &callbacks);

    CGPoint startPoint = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
    CGFloat startRadius = 0.0;
    CGPoint endPoint = startPoint;
    CGFloat endRadius = MAX(CGRectGetWidth(self.bounds), CGRectGetHeight(self.bounds)) / 2;

    CGShadingRef shading = CGShadingCreateRadial(colorSpace, startPoint, startRadius, endPoint,
                                                 endRadius, shadingFunction,
                                                 YES,   // extend shading beyond starting circle
                                                 YES);  // extend shading beyond ending circle
    CGContextDrawShading(context, shading);

    CGShadingRelease(shading);
    CGFunctionRelease(shadingFunction);
    CGColorSpaceRelease(colorSpace);
}

#define kGradientMaximumAlphaValue  0.90
#define kGradientAlphaDecayFactor   1.1263

static void exponentialDecayInterpolation(void *info, const CGFloat *input, CGFloat *output)
{
    // output is an RGBA array corresponding to the color black with an alpha value somewhere on
    // our exponential decay curve.
    CGFloat progress = *input;
    output[0] = 0.0;
    output[1] = 0.0;
    output[2] = 0.0;
    output[3] = kGradientMaximumAlphaValue - exp(-progress / kGradientAlphaDecayFactor);
}

- (void)layoutSubviews
{
    [self updateCloseButtonPosition];
}

- (void)updateCloseButtonPosition
{
    // Ensure that the close button is anchored to the top-right corner of the screen.

    CGPoint originalCenter = _closeButtonPortraitCenter;
    CGPoint center = originalCenter;
    BOOL statusBarHidden = [UIApplication sharedApplication].statusBarHidden;
    CGFloat statusBarOffset = (statusBarHidden) ? 0.0 : 20.0;

    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    switch (orientation) {
        case UIInterfaceOrientationLandscapeLeft:
            center.x = CGRectGetMaxX(self.bounds) - originalCenter.x + statusBarOffset;
            center.y = originalCenter.y;
            break;
        case UIInterfaceOrientationLandscapeRight:
            center.x = originalCenter.x - statusBarOffset;
            center.y = CGRectGetMaxY(self.bounds) - originalCenter.y;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            center.x = CGRectGetMaxX(self.bounds) - originalCenter.x;
            center.y = CGRectGetMaxY(self.bounds) - originalCenter.y - statusBarOffset;
            break;
        default:
            center.y = originalCenter.y + statusBarOffset;
            break;
    }

    _closeButton.center = center;
}

#pragma mark - Internal

- (void)registerForDeviceOrientationNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(deviceOrientationDidChange:)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
}

- (void)unregisterForDeviceOrientationNotifications
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIDeviceOrientationDidChangeNotification
                                                  object:nil];
}

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    [self setTransformForCurrentOrientationAnimated:YES];
    [self setNeedsLayout];
}

- (void)enableCloseButton
{
    _closeButton.hidden = NO;

    [UIView beginAnimations:nil context:nil];
    _closeButton.alpha = 1.0;
    [UIView commitAnimations];
}

- (void)closeButtonPressed
{
    if ([_delegate respondsToSelector:@selector(overlayCancelButtonPressed)]) {
        [_delegate overlayCancelButtonPressed];
    }
}

- (void)setTransformForCurrentOrientationAnimated:(BOOL)animated
{
    UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
    float angle = 0;
    if (UIInterfaceOrientationIsPortrait(orientation)) {
        if (orientation == UIInterfaceOrientationPortraitUpsideDown) {
            angle = M_PI;
        }
    } else {
        if (orientation == UIInterfaceOrientationLandscapeLeft) {
            angle = -M_PI_2;
        } else {
            angle = M_PI_2;
        }
    }

    if (animated) {
        [UIView beginAnimations:nil context:nil];
        [self setTransformForAllSubviews:CGAffineTransformMakeRotation(angle)];
        [UIView commitAnimations];
    } else {
        [self setTransformForAllSubviews:CGAffineTransformMakeRotation(angle)];
    }
}

- (void)setTransformForAllSubviews:(CGAffineTransform)transform
{
    for (UIView *view in self.subviews) {
        view.transform = transform;
    }
}

@end
