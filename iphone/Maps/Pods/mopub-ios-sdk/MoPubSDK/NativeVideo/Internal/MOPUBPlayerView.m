//
//  MOPUBPlayerView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPGlobal.h"
#import "MPLogging.h"
#import "MOPUBAVPlayerView.h"
#import "MOPUBAVPlayer.h"
#import "MOPUBAVPlayerView.h"
#import "MOPUBPlayerView.h"
#import "MOPUBPlayerViewController.h"
#import "MOPUBReplayView.h"
#import "UIView+MPAdditions.h"
#import "UIColor+MPAdditions.h"

static NSString * const kProgressBarFillColor = @"#FFCC4D";
static CGFloat const kVideoProgressBarHeight = 4.0f;

// gradient
static NSString * const kTopGradientColor = @"#000000";
static NSString * const kBottomGradientColor = @"#000000";
static CGFloat const kTopGradientAlpha = 0.0f;
static CGFloat const kBottomGradientAlpha = 0.4f;
static CGFloat const kGradientViewHeight = 25.0f;

@interface MOPUBPlayerView()

// UI elements
@property (nonatomic) MOPUBAVPlayerView *avView;
@property (nonatomic) MOPUBReplayView *replayView;
@property (nonatomic) UIButton *replayVideoButton;
@property (nonatomic) UIView *progressBarBackground;
@property (nonatomic) UIView *progressBar;
@property (nonatomic) UIView *gradientView;
@property (nonatomic) CAGradientLayer *gradient;

@property (nonatomic) UITapGestureRecognizer *tapGestureRecognizer;
@property (nonatomic, weak) id<MOPUBPlayerViewDelegate> delegate;

@end

@implementation MOPUBPlayerView

- (instancetype)initWithFrame:(CGRect)frame delegate:(id<MOPUBPlayerViewDelegate>)delegate
{
    self = [super initWithFrame:frame];
    if (self) {
        _delegate = delegate;

        _tapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(avPlayerTapped)];
        [self addGestureRecognizer:_tapGestureRecognizer];

        self.accessibilityLabel = @"MoPub Native Video";
    }
    return self;
}

- (void)dealloc
{
    [self.tapGestureRecognizer removeTarget:self action:@selector(avPlayerTapped)];
}

- (void)createPlayerView
{
    self.clipsToBounds = YES;
    if (!self.gradientView && self.displayMode == MOPUBPlayerDisplayModeInline) {
        // Create the gradient
        self.gradientView = [UIView new];
        UIColor *topColor = [UIColor mp_colorFromHexString:kTopGradientColor alpha:kTopGradientAlpha];
        UIColor *bottomColor = [UIColor mp_colorFromHexString:kBottomGradientColor alpha:kBottomGradientAlpha];
        self.gradient = [CAGradientLayer layer];
        self.gradient.colors = [NSArray arrayWithObjects: (id)topColor.CGColor, (id)bottomColor.CGColor, nil];
        self.gradient.frame = CGRectMake(0, 0, CGRectGetWidth(self.bounds), kGradientViewHeight);

        //Add gradient to view
        [self.gradientView.layer insertSublayer:self.gradient atIndex:0];
        [self addSubview:self.gradientView];
    }

    if (!self.progressBar) {
        self.progressBar = [[UIView alloc] init];
        self.progressBarBackground = [[UIView alloc] init];
        [self addSubview:self.progressBarBackground];

        self.progressBarBackground.backgroundColor = [UIColor colorWithRed:.5f green:.5f blue:.5f alpha:.5f];
        self.progressBar.backgroundColor = [UIColor mp_colorFromHexString:kProgressBarFillColor alpha:1.0f];
        [self addSubview:self.progressBar];
    }
}

#pragma mark - set avPlayer

- (void)setAvPlayer:(MOPUBAVPlayer *)player
{
    if (!player) {
        MPLogInfo(@"Cannot set avPlayer to nil");
        return;
    }
    if (_avPlayer == player) {
        return;
    }
    _avPlayer = player;
    [_avView removeFromSuperview];
    _avView = [[MOPUBAVPlayerView alloc] initWithFrame:CGRectZero];
    _avView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    self.videoGravity = AVLayerVideoGravityResizeAspectFill;
    self.avView.player = self.avPlayer;
    self.avView.frame = (CGRect){CGPointZero, self.bounds.size};
    [self insertSubview:_avView atIndex:0];
}

- (void)setVideoGravity:(NSString *)videoGravity
{
    ((AVPlayerLayer *)_avView.layer).videoGravity = videoGravity;
}

// make the player view not clickable when initializing video failed.
- (void)handleVideoInitFailure
{
    [self removeGestureRecognizer:self.tapGestureRecognizer];
}

#pragma mark - Synchronize UI Elements

- (void)playbackTimeDidProgress
{
    [self layoutProgressBar];
}

- (void)playbackDidFinish
{
    if (!self.replayView) {
        self.replayView = [[MOPUBReplayView alloc] initWithFrame:self.avView.bounds displayMode:self.displayMode];
        __weak __typeof__(self) weakSelf = self;
        self.replayView.actionBlock = ^(MOPUBReplayView *view) {
            __strong __typeof__(self) strongSelf = weakSelf;
            if ([strongSelf.delegate respondsToSelector:@selector(playerViewDidTapReplayButton:)]) {
                [strongSelf.delegate playerViewDidTapReplayButton:strongSelf];
            }
            [strongSelf.replayView removeFromSuperview];
            strongSelf.replayView = nil;
        };
        [self addSubview:self.replayView];

        if ([self.delegate respondsToSelector:@selector(playerViewWillShowReplayView:)]) {
            [self.delegate playerViewWillShowReplayView:self];
        }
    }
}

- (void)setProgressBarVisible:(BOOL)visible
{
    self.progressBarBackground.hidden = !visible;
    self.progressBar.hidden = !visible;
}

#pragma mark - Touch event

- (void)avPlayerTapped
{
    // Only trigger tap event in infeed mode
    if (self.displayMode == MOPUBPlayerDisplayModeInline) {
        self.displayMode = MOPUBPlayerDisplayModeFullscreen;
        if ([self.delegate respondsToSelector:@selector(playerViewWillEnterFullscreen:)]) {
            [self.delegate playerViewWillEnterFullscreen:self];
        }
        [self setNeedsLayout];
        [self layoutIfNeeded];
    }
}

#pragma mark - layout views

- (void)layoutSubviews
{
    [super layoutSubviews];

    [self layoutProgressBar];
    [self layoutGradientview];
    [self layoutReplayView];
}

- (void)layoutProgressBar
{
    if (self.avPlayer && !isnan(self.avPlayer.currentItemDuration)) {
        CGFloat vcWidth = CGRectGetWidth(self.bounds);
        CGFloat currentProgress = self.avPlayer.currentPlaybackTime/self.avPlayer.currentItemDuration;
        if (currentProgress < 0) {
            currentProgress = 0;
            MPLogInfo(@"Progress shouldn't be < 0");
        }
        if (currentProgress > 1) {
            currentProgress = 1;
            MPLogInfo(@"Progress shouldn't be > 1");
        }

        self.progressBar.frame = CGRectMake(0, CGRectGetMaxY(self.avView.frame)- kVideoProgressBarHeight, vcWidth * currentProgress, kVideoProgressBarHeight);
        self.progressBarBackground.frame = CGRectMake(0, CGRectGetMaxY(self.avView.frame) - kVideoProgressBarHeight, vcWidth, kVideoProgressBarHeight);
    }
}


- (void)layoutGradientview
{
    if (self.displayMode == MOPUBPlayerDisplayModeInline) {
        self.gradientView.hidden = NO;
        self.gradient.frame = CGRectMake(0, 0, CGRectGetWidth(self.bounds), kGradientViewHeight);
        self.gradientView.frame = CGRectMake(0, CGRectGetMaxY(self.avView.frame) - kGradientViewHeight, CGRectGetWidth(self.bounds),  kGradientViewHeight);
    } else {
        self.gradientView.hidden = YES;
    }
}

- (void)layoutReplayView
{
    if (self.replayView) {
        CGSize appFrameSize = MPApplicationFrame(YES).size;
        BOOL isLandscapeOrientation = appFrameSize.width > appFrameSize.height;

        if (isLandscapeOrientation && self.displayMode == MOPUBPlayerDisplayModeFullscreen) {
            self.replayView.frame = CGRectMake(0, 0, appFrameSize.width, appFrameSize.height);
        } else {
            self.replayView.frame = self.avView.frame;
        }
        [self.replayView setNeedsLayout];
        [self.replayView layoutIfNeeded];
    }
}

@end
