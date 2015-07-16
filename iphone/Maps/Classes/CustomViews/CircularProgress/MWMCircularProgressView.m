//
//  MWMCircularProgressView.m
//  Maps
//
//  Created by Ilya Grechuhin on 11.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

static CGFloat const kLineWidth = 2.0;
static NSString * const kAnimationKey = @"CircleAnimation";

static inline CGFloat angleWithProgress(CGFloat progress)
{
  return 2.0 * M_PI * progress - M_PI_2;
}

@interface MWMCircularProgressView ()

@property (nonatomic) CAShapeLayer * backgroundLayer;
@property (nonatomic) CAShapeLayer * progressLayer;

@property (weak, nonatomic) IBOutlet MWMCircularProgress * owner;

@end

@implementation MWMCircularProgressView

- (void)awakeFromNib
{
  self.backgroundLayer = [CAShapeLayer layer];
  [self refreshBackground];
  [self.layer addSublayer:self.backgroundLayer];

  self.progressLayer = [CAShapeLayer layer];
  [self refreshProgress];
  [self.layer addSublayer:self.progressLayer];
}

- (void)refreshBackground
{
  self.backgroundLayer.fillColor = [UIColor clearColor].CGColor;
  self.backgroundLayer.lineWidth = kLineWidth;
  self.backgroundLayer.strokeColor = [UIColor pressBackground].CGColor;
  CGRect rect = CGRectInset(self.bounds, kLineWidth, kLineWidth);
  self.backgroundLayer.path = [UIBezierPath bezierPathWithOvalInRect:rect].CGPath;
}

- (void)refreshProgress
{
  self.progressLayer.fillColor = [UIColor clearColor].CGColor;
  self.progressLayer.lineWidth = kLineWidth;
  self.progressLayer.strokeColor = self.owner.failed ? [UIColor red].CGColor : [UIColor primary].CGColor;
}

- (void)updatePath:(CGFloat)progress
{
  CGFloat const outerRadius = self.width / 2.0;
  CGPoint const center = CGPointMake(outerRadius, outerRadius);
  CGFloat const radius = outerRadius - kLineWidth;
  UIBezierPath * path = [UIBezierPath bezierPathWithArcCenter:center radius:radius startAngle:angleWithProgress(0.0) endAngle:angleWithProgress(progress) clockwise:YES];
  self.progressLayer.path = path.CGPath;
}

#pragma mark - Animation

- (void)animateFromValue:(CGFloat)fromValue toValue:(CGFloat)toValue
{
  [self updatePath:toValue];
  CABasicAnimation * animation = [CABasicAnimation animationWithKeyPath:@"strokeEnd"];
  animation.duration = 0.3;
  animation.repeatCount = 1;
  animation.fromValue = @(fromValue / toValue);
  animation.toValue = @1;
  animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
  animation.delegate = self.owner;
  [self.progressLayer addAnimation:animation forKey:kAnimationKey];
}

- (void)layoutSubviews
{
  self.frame = self.superview.bounds;
  [super layoutSubviews];
}

#pragma mark - Properties

- (void)setFrame:(CGRect)frame
{
  BOOL const needRefreshBackground = !CGRectEqualToRect(self.frame, frame);
  super.frame = frame;
  if (needRefreshBackground)
    [self refreshBackground];
}

- (BOOL)animating
{
  return [self.progressLayer animationForKey:kAnimationKey] != nil;
}

@end
