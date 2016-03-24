#import "Common.h"
#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"

#include "std/map.hpp"

static CGFloat const kLineWidth = 2.0;
static NSString * const kAnimationKey = @"CircleAnimation";

static inline CGFloat angleWithProgress(CGFloat progress)
{
  return 2.0 * M_PI * progress - M_PI_2;
}

@interface MWMCircularProgressView ()

@property (nonatomic) CAShapeLayer * backgroundLayer;
@property (nonatomic) CAShapeLayer * progressLayer;

@property (nonatomic, readonly) CGColorRef backgroundLayerColor;
@property (nonatomic, readonly) CGColorRef progressLayerColor;

@property (nonatomic) NSMutableDictionary * images;
@property (nonatomic) NSMutableDictionary * colors;

@property (weak, nonatomic) IBOutlet MWMCircularProgress * owner;
@property (weak, nonatomic) IBOutlet UIImageView * spinner;
@property (weak, nonatomic) IBOutlet MWMButton * button;

@end

@implementation MWMCircularProgressView
{
  map<MWMCircularProgressState, MWMButtonColoring> m_buttonColoring;
}

- (void)awakeFromNib
{
  self.images = [NSMutableDictionary dictionary];
  [self setupColors];
  [self setupButtonColoring];
  [self setupAnimationLayers];
}

- (void)layoutSubviews
{
  self.frame = self.superview.bounds;
  [super layoutSubviews];
}

#pragma mark - Setup

- (void)setupColors
{
  self.colors = [NSMutableDictionary dictionary];
  self.spinner.mwm_coloring = MWMImageColoringGray;
  UIColor * progressColor = [UIColor linkBlue];
  UIColor * clearColor = [UIColor clearColor];
  [self setColor:clearColor forState:MWMCircularProgressStateNormal];
  [self setColor:clearColor forState:MWMCircularProgressStateSelected];
  [self setColor:progressColor forState:MWMCircularProgressStateProgress];
  [self setColor:progressColor forState:MWMCircularProgressStateSpinner];
  [self setColor:clearColor forState:MWMCircularProgressStateFailed];
  [self setColor:clearColor forState:MWMCircularProgressStateCompleted];
}

- (void)setupButtonColoring
{
  [self setColoring:MWMButtonColoringBlack forState:MWMCircularProgressStateNormal];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateSelected];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateProgress];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateSpinner];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateFailed];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateCompleted];
}

- (void)mwm_refreshUI
{
  [self setupColors];
}

- (void)setupAnimationLayers
{
  self.backgroundLayer = [CAShapeLayer layer];
  self.progressLayer = [CAShapeLayer layer];

  [self refreshProgress];
  [self.layer addSublayer:self.backgroundLayer];
  [self.layer addSublayer:self.progressLayer];
}

- (void)setImage:(nonnull UIImage *)image forState:(MWMCircularProgressState)state
{
  self.images[@(state)] = image;
  [self refreshProgress];
}

- (void)setColor:(nonnull UIColor *)color forState:(MWMCircularProgressState)state
{
  self.colors[@(state)] = color;
  [self refreshProgress];
}

- (void)setColoring:(MWMButtonColoring)coloring forState:(MWMCircularProgressState)state
{
  m_buttonColoring[state] = coloring;
  [self refreshProgress];
}

#pragma mark - Progress

- (void)refreshProgress
{
  self.backgroundLayer.fillColor = self.progressLayer.fillColor = UIColor.clearColor.CGColor;
  self.backgroundLayer.lineWidth = self.progressLayer.lineWidth = kLineWidth;
  self.backgroundLayer.strokeColor = self.backgroundLayerColor;
  self.progressLayer.strokeColor = self.progressLayerColor;
  CGRect rect = CGRectInset(self.bounds, kLineWidth, kLineWidth);
  self.backgroundLayer.path = [UIBezierPath bezierPathWithOvalInRect:rect].CGPath;
  [self.button setImage:self.images[@(self.state)] forState:UIControlStateNormal];
  self.button.coloring = m_buttonColoring[self.state];
}

- (void)updatePath:(CGFloat)progress
{
  if (progress > 0.0)
  {
    self.state = progress < 1.0 ? MWMCircularProgressStateProgress : MWMCircularProgressStateCompleted;
    [self stopSpinner];
  }
  CGFloat const outerRadius = self.width / 2.0;
  CGPoint const center = {outerRadius, outerRadius};
  CGFloat const radius = outerRadius - kLineWidth;
  UIBezierPath * path = [UIBezierPath bezierPathWithArcCenter:center
                                                       radius:radius
                                                   startAngle:angleWithProgress(0.0)
                                                     endAngle:angleWithProgress(progress)
                                                    clockwise:YES];
  self.progressLayer.path = path.CGPath;
}

#pragma mark - Spinner

- (void)startSpinner
{
  if (self.spinner.hidden)
  {
    self.spinner.hidden = NO;
    self.backgroundLayer.hidden = self.progressLayer.hidden = YES;
  }
  NSUInteger const animationImagesCount = 12;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  NSString * postfix = ([UIColor isNightMode] && !self.isInvertColor) || (![UIColor isNightMode] && self.isInvertColor) ? @"dark" : @"light";
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@_%@", @(i+1), postfix]];

  self.spinner.animationDuration = 0.8;
  self.spinner.animationImages = animationImages;
  [self.spinner startAnimating];
}

- (void)stopSpinner
{
  if (self.spinner.hidden)
    return;
  self.spinner.hidden = YES;
  self.backgroundLayer.hidden = self.progressLayer.hidden = NO;
  [self.spinner stopAnimating];
}

#pragma mark - Animation

- (void)animateFromValue:(CGFloat)fromValue toValue:(CGFloat)toValue
{
  [self updatePath:toValue];
  CABasicAnimation * animation = [CABasicAnimation animationWithKeyPath:@"strokeEnd"];
  animation.duration = kDefaultAnimationDuration;
  animation.repeatCount = 1;
  animation.fromValue = @(fromValue / toValue);
  animation.toValue = @1;
  animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
  animation.delegate = self.owner;
  [self.progressLayer addAnimation:animation forKey:kAnimationKey];
}

#pragma mark - Properties

- (void)setState:(MWMCircularProgressState)state
{
  if (state == MWMCircularProgressStateSpinner)
    [self startSpinner];
  else if (_state == MWMCircularProgressStateSpinner)
    [self stopSpinner];
  if (_state == state)
    return;
  _state = state;
  [self refreshProgress];
}

- (CGColorRef)backgroundLayerColor
{
  switch (self.state)
  {
    case MWMCircularProgressStateProgress:
      return [UIColor pressBackground].CGColor;
    default:
      return [UIColor clearColor].CGColor;
  }
}

- (CGColorRef)progressLayerColor
{
  UIColor * color = self.colors[@(self.state)];
  return color.CGColor;
}

- (void)setFrame:(CGRect)frame
{
  BOOL const needrefreshProgress = !CGRectEqualToRect(self.frame, frame);
  super.frame = frame;
  if (needrefreshProgress)
    [self refreshProgress];
}

- (BOOL)animating
{
  return [self.progressLayer animationForKey:kAnimationKey] != nil;
}

@end
