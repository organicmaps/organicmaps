//
//  MPBReplayView.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBReplayView.h"
#import "MPGlobal.h"
#import "UIView+MPAdditions.h"
#import "UIColor+MPAdditions.h"

static NSString * const kPlayButtonImage = @"MPPlayBtn.png";
static NSString * const kOverlayBgColor = @"#000000";
static CGFloat const kOverlayAlpha = 0.5f;

@interface MOPUBReplayView()

@property (nonatomic) UIView *overlayView;
@property (nonatomic) UIButton *replayVideoButton;

@end

@implementation MOPUBReplayView

- (instancetype)initWithFrame:(CGRect)frame displayMode:(MOPUBPlayerDisplayMode)displayMode
{
    if (self = [super initWithFrame:frame]) {
        // only apply the overlay for fullscreen mode
        if (displayMode == MOPUBPlayerDisplayModeFullscreen) {
            _overlayView = [UIView new];
            _overlayView.backgroundColor = [UIColor mp_colorFromHexString:kOverlayBgColor alpha:kOverlayAlpha];
            [self addSubview:_overlayView];
        }

        _replayVideoButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [_replayVideoButton setImage:[UIImage imageNamed:MPResourcePathForResource(kPlayButtonImage)] forState:UIControlStateNormal];
        [_replayVideoButton addTarget:self action:@selector(replayButtonTapped) forControlEvents:UIControlEventTouchUpInside];
        [_replayVideoButton sizeToFit];
        [self addSubview:_replayVideoButton];
    }
    return self;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    self.overlayView.frame = self.bounds;
    self.replayVideoButton.center = self.center;
    self.replayVideoButton.frame = CGRectIntegral(self.replayVideoButton.frame);
}

- (void)replayButtonTapped
{
    self.actionBlock(self);
}

@end
