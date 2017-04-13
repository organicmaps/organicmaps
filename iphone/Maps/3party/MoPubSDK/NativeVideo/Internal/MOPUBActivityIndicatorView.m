//
//  MOPUBActivityIndicatorView.m
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MOPUBActivityIndicatorView.h"
#import "MPGlobal.h"
#import "UIColor+MPAdditions.h"

static NSString * const kSpinnerBgColor = @"#000000";
static CGFloat const kSpinnerAlpha = 0.5f;
static CGFloat const kSpinnerCornerRadius = 4.0f;

@interface MOPUBActivityIndicatorView()

@property (nonatomic) UIActivityIndicatorView *activityIndicator;
@property (nonatomic) UIView *bgView;

@end

@implementation MOPUBActivityIndicatorView

- (instancetype)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) {
        _bgView = [[UIView alloc] initWithFrame:frame];
        _bgView.backgroundColor = [UIColor mp_colorFromHexString:kSpinnerBgColor alpha:kSpinnerAlpha];
        _bgView.layer.cornerRadius = kSpinnerCornerRadius;
        [self addSubview:_bgView];

        _activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
        _activityIndicator.center = self.center;
        _activityIndicator.frame = CGRectIntegral(_activityIndicator.frame);
        [self addSubview:_activityIndicator];
    }
    return self;
}

- (void)startAnimating
{
    self.bgView.hidden = NO;
    [self.activityIndicator startAnimating];
}

- (void)stopAnimating
{
    self.bgView.hidden = YES;
    [self.activityIndicator stopAnimating];
}

- (BOOL)isAnimating
{
    return self.activityIndicator.isAnimating;
}

@end
