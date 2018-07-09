//
//  MPClosableView.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPClosableView.h"
#import "MPInstanceProvider.h"
#import "MPUserInteractionGestureRecognizer.h"

static CGFloat kCloseRegionWidth = 50.0f;
static CGFloat kCloseRegionHeight = 50.0f;
static NSString *const kExpandableCloseButtonImageName = @"MPCloseButtonX.png";

CGRect MPClosableViewCustomCloseButtonFrame(CGSize size, MPClosableViewCloseButtonLocation closeButtonLocation)
{
    CGFloat width = size.width;
    CGFloat height = size.height;
    CGRect closeButtonFrame = CGRectMake(0.0f, 0.0f, kCloseRegionWidth, kCloseRegionHeight);

    switch (closeButtonLocation) {
        case MPClosableViewCloseButtonLocationTopRight:
            closeButtonFrame.origin = CGPointMake(width-kCloseRegionWidth, 0.0f);
            break;
        case MPClosableViewCloseButtonLocationTopLeft:
            closeButtonFrame.origin = CGPointMake(0.0f, 0.0f);
            break;
        case MPClosableViewCloseButtonLocationTopCenter:
            closeButtonFrame.origin = CGPointMake((width-kCloseRegionWidth) / 2.0f, 0.0f);
            break;
        case MPClosableViewCloseButtonLocationBottomRight:
            closeButtonFrame.origin = CGPointMake(width-kCloseRegionWidth, height-kCloseRegionHeight);
            break;
        case MPClosableViewCloseButtonLocationBottomLeft:
            closeButtonFrame.origin = CGPointMake(0.0f, height-kCloseRegionHeight);
            break;
        case MPClosableViewCloseButtonLocationBottomCenter:
            closeButtonFrame.origin = CGPointMake((width-kCloseRegionWidth) / 2.0f, height-kCloseRegionHeight);
            break;
        case MPClosableViewCloseButtonLocationCenter:
            closeButtonFrame.origin = CGPointMake((width-kCloseRegionWidth) / 2.0f, (height-kCloseRegionHeight) / 2.0f);
            break;
        default:
            closeButtonFrame.origin = CGPointMake(width-kCloseRegionWidth, 0.0f);
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

- (instancetype)initWithFrame:(CGRect)frame closeButtonType:(MPClosableViewCloseButtonType)closeButtonType
{
    self = [super initWithFrame:frame];

    if (self) {
        self.backgroundColor = [UIColor clearColor];
        self.opaque = NO;

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

        [self setCloseButtonType:closeButtonType];

        [self addSubview:_closeButton];
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
    if (@available(iOS 11, *)) {
        self.closeButton.translatesAutoresizingMaskIntoConstraints = NO;

        NSMutableArray <NSLayoutConstraint *> *constraints = [NSMutableArray arrayWithObjects:
                                                              [self.closeButton.widthAnchor constraintEqualToConstant:kCloseRegionWidth],
                                                              [self.closeButton.heightAnchor constraintEqualToConstant:kCloseRegionHeight],
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

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer;
{
    return YES;
}

#pragma mark - <UIButton>

- (void)closeButtonPressed
{
    [self.delegate closeButtonPressed:self];
}

@end
