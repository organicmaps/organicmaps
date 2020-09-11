//
//  MPClosableView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPClosableView.h"
#import "MPGlobal.h"
#import "MPUserInteractionGestureRecognizer.h"

/**
 Per MRAID spec https://www.iab.com/wp-content/uploads/2015/08/IAB_MRAID_v2_FINAL.pdf, page 31, the
 close event region should be 50x50 expandable and interstitial ads. On page 34, the 50x50 size applies
 to resized ads as well.
 */
const CGSize kCloseRegionSize = {.width = 50, .height = 50};

static NSString *const kExpandableCloseButtonImageName = @"MPCloseButtonX.png";

/**
 Provided the ad size and close button location, returns the frame of the close button.
 Note: The provided ad size is assumed to be at least 50x50, otherwise the return value is undefined.
 @param adSize The size of the ad
 @param closeButtonLocation The location of the close button
 */
CGRect MPClosableViewCustomCloseButtonFrame(CGSize adSize, MPClosableViewCloseButtonLocation closeButtonLocation)
{
    CGRect closeButtonFrame = CGRectMake(0.0f, 0.0f, kCloseRegionSize.width, kCloseRegionSize.height);

    switch (closeButtonLocation) {
        case MPClosableViewCloseButtonLocationTopRight:
            closeButtonFrame.origin = CGPointMake(adSize.width-kCloseRegionSize.width, 0.0f);
            break;
        case MPClosableViewCloseButtonLocationTopLeft:
            closeButtonFrame.origin = CGPointMake(0.0f, 0.0f);
            break;
        case MPClosableViewCloseButtonLocationTopCenter:
            closeButtonFrame.origin = CGPointMake((adSize.width-kCloseRegionSize.width) / 2.0f, 0.0f);
            break;
        case MPClosableViewCloseButtonLocationBottomRight:
            closeButtonFrame.origin = CGPointMake(adSize.width-kCloseRegionSize.width, adSize.height-kCloseRegionSize.height);
            break;
        case MPClosableViewCloseButtonLocationBottomLeft:
            closeButtonFrame.origin = CGPointMake(0.0f, adSize.height-kCloseRegionSize.height);
            break;
        case MPClosableViewCloseButtonLocationBottomCenter:
            closeButtonFrame.origin = CGPointMake((adSize.width-kCloseRegionSize.width) / 2.0f, adSize.height-kCloseRegionSize.height);
            break;
        case MPClosableViewCloseButtonLocationCenter:
            closeButtonFrame.origin = CGPointMake((adSize.width-kCloseRegionSize.width) / 2.0f, (adSize.height-kCloseRegionSize.height) / 2.0f);
            break;
        default: // top right
            closeButtonFrame.origin = CGPointMake(adSize.width-kCloseRegionSize.width, 0.0f);
            break;
    }

    return closeButtonFrame;
}

@interface MPClosableView () <UIGestureRecognizerDelegate>

@property (nonatomic, strong, readwrite) UIButton *closeButton;
@property (nonatomic, strong) UIImage *closeButtonImage;
@property (nonatomic, strong) MPUserInteractionGestureRecognizer *userInteractionRecognizer;
@property (nonatomic, assign) BOOL wasTapped;

@end

@implementation MPClosableView

- (instancetype)initWithFrame:(CGRect)frame
                  contentView:(UIView *)contentView
                     delegate:(id<MPClosableViewDelegate>)delegate {
    self = [super initWithFrame:frame];

    if (self) {
        self.backgroundColor = [UIColor clearColor];
        self.opaque = NO;
        self.clipsToBounds = YES;

        _delegate = delegate;

        // Set up close button
        _closeButtonLocation = MPClosableViewCloseButtonLocationTopRight;

        _userInteractionRecognizer = [[MPUserInteractionGestureRecognizer alloc] initWithTarget:self action:@selector(handleInteraction:)];
        _userInteractionRecognizer.cancelsTouchesInView = NO;
        [self addGestureRecognizer:_userInteractionRecognizer];
        _userInteractionRecognizer.delegate = self;

        _closeButtonImage = [UIImage imageNamed:MPResourcePathForResource(kExpandableCloseButtonImageName)];

        _closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
        _closeButton.backgroundColor = [UIColor clearColor];
        _closeButton.accessibilityLabel = @"Close Interstitial Ad";

        [_closeButton addTarget:self action:@selector(closeButtonPressed) forControlEvents:UIControlEventTouchUpInside];

        [self setCloseButtonType:MPClosableViewCloseButtonTypeTappableWithImage];

        [self addSubview:_closeButton];

        // Set up web view
        contentView.frame = self.bounds;
        [self addSubview:contentView];
    }

    return self;
}

- (void)dealloc
{
    _userInteractionRecognizer.delegate = nil;
    [self.userInteractionRecognizer removeTarget:self action:nil];
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    if (@available(iOS 11, *)) {
        self.closeButton.translatesAutoresizingMaskIntoConstraints = NO;

        NSMutableArray <NSLayoutConstraint *> *constraints = [NSMutableArray arrayWithObjects:
                                                              [self.closeButton.widthAnchor constraintEqualToConstant:kCloseRegionSize.width],
                                                              [self.closeButton.heightAnchor constraintEqualToConstant:kCloseRegionSize.height],
                                                              nil];

        switch (self.closeButtonLocation) {
            case MPClosableViewCloseButtonLocationTopRight:
                [constraints addObject:[self.closeButton.trailingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.trailingAnchor]];
                [constraints addObject:[self.closeButton.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor]];
                break;
            case MPClosableViewCloseButtonLocationTopLeft:
                [constraints addObject:[self.closeButton.leadingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.leadingAnchor]];
                [constraints addObject:[self.closeButton.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor]];
                break;
            case MPClosableViewCloseButtonLocationTopCenter:
                [constraints addObject:[self.closeButton.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor]];
                [constraints addObject:[self.closeButton.centerXAnchor constraintEqualToAnchor:self.centerXAnchor]];
                break;
            case MPClosableViewCloseButtonLocationBottomRight:
                [constraints addObject:[self.closeButton.trailingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.trailingAnchor]];
                [constraints addObject:[self.closeButton.bottomAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.bottomAnchor]];
                break;
            case MPClosableViewCloseButtonLocationBottomLeft:
                [constraints addObject:[self.closeButton.leadingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.leadingAnchor]];
                [constraints addObject:[self.closeButton.bottomAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.bottomAnchor]];
                break;
            case MPClosableViewCloseButtonLocationBottomCenter:
                [constraints addObject:[self.closeButton.bottomAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.bottomAnchor]];
                [constraints addObject:[self.closeButton.centerXAnchor constraintEqualToAnchor:self.centerXAnchor]];
                break;
            case MPClosableViewCloseButtonLocationCenter:
                [constraints addObject:[self.closeButton.centerXAnchor constraintEqualToAnchor:self.centerXAnchor]];
                [constraints addObject:[self.closeButton.centerYAnchor constraintEqualToAnchor:self.centerYAnchor]];
                break;
            default:
                // Top-right default
                [constraints addObject:[self.closeButton.trailingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.trailingAnchor]];
                [constraints addObject:[self.closeButton.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor]];
                break;
        }

        [NSLayoutConstraint activateConstraints:constraints];
    } else {
        self.closeButton.frame = MPClosableViewCustomCloseButtonFrame(self.bounds.size, self.closeButtonLocation);
    }

    [self bringSubviewToFront:self.closeButton];
}

- (void)didMoveToWindow
{
    if ([self.delegate respondsToSelector:@selector(closableView:didMoveToWindow:)]) {
        [self.delegate closableView:self didMoveToWindow:self.window];
    }
}

- (void)setCloseButtonType:(MPClosableViewCloseButtonType)closeButtonType
{
    _closeButtonType = closeButtonType;

    switch (closeButtonType) {
        case MPClosableViewCloseButtonTypeNone:
            self.closeButton.hidden = YES;
            break;
        case MPClosableViewCloseButtonTypeTappableWithoutImage:
            self.closeButton.hidden = NO;
            [self.closeButton setImage:nil forState:UIControlStateNormal];
            break;
        case MPClosableViewCloseButtonTypeTappableWithImage:
            self.closeButton.hidden = NO;
            [self.closeButton setImage:self.closeButtonImage forState:UIControlStateNormal];
            break;
        default:
            self.closeButton.hidden = NO;
            [self.closeButton setImage:self.closeButtonImage forState:UIControlStateNormal];
            break;
    }
}

- (void)setCloseButtonLocation:(MPClosableViewCloseButtonLocation)closeButtonLocation
{
    _closeButtonLocation = closeButtonLocation;
    [self setNeedsLayout];
}

- (void)handleInteraction:(UIGestureRecognizer *)gestureRecognizer
{
    if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
        self.wasTapped = YES;
    }
}

#pragma mark - <UIGestureRecognizerDelegate>

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return YES;
}

#pragma mark - <UIButton>

- (void)closeButtonPressed
{
    [self.delegate closeButtonPressed:self];
}

@end
