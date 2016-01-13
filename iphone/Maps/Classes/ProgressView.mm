#import "ProgressView.h"
#import "UIColor+MapsMeColor.h"

@interface ProgressView ()

@property (nonatomic) CAShapeLayer * progressLayer;
@property (nonatomic) CAShapeLayer * backgroundCircleLayer;
@property (nonatomic) CAShapeLayer * stopRectLayer;
@property (nonatomic) UIImageView * startTriangleView;
@property (nonatomic) UIButton * button;
@property (nonatomic) NSNumber * nextProgressToAnimate;

@end

@implementation ProgressView

#define CIRCLE_RADIUS 18

- (instancetype)init
{
  self = [super initWithFrame:CGRectMake(0, 0, 44, 44)];

  [self.layer addSublayer:self.backgroundCircleLayer];
  [self.layer addSublayer:self.progressLayer];
  [self.layer addSublayer:self.stopRectLayer];
  [self addSubview:self.startTriangleView];
  self.startTriangleView.center = CGPointMake(self.width / 2 + 1, self.height / 2);
  [self addSubview:self.button];

  [self setProgress:0 animated:NO];

  return self;
}

- (void)buttonPressed:(id)sender
{
  if (self.failedMode)
    [self.delegate progressViewDidStart:self];
  else
    [self.delegate progressViewDidCancel:self];
}

- (CGFloat)angleWithProgress:(double)progress
{
  return 2 * M_PI * progress - M_PI_2;
}

NSString * const CircleAnimationKey = @"CircleAnimation";

- (void)setProgress:(double)progress animated:(BOOL)animated
{
  if ([self.progressLayer animationForKey:CircleAnimationKey])
  {
    self.nextProgressToAnimate = @(progress);
  }
  else
  {
    self.nextProgressToAnimate = nil;
    CGPoint const center = CGPointMake(self.width / 2, self.height / 2);
    CGFloat const radius = CIRCLE_RADIUS - self.progressLayer.lineWidth;
    UIBezierPath * path = [UIBezierPath bezierPathWithArcCenter:center radius:radius startAngle:[self angleWithProgress:0] endAngle:[self angleWithProgress:progress] clockwise:YES];
    if (animated)
    {
      self.progressLayer.path = path.CGPath;

      CABasicAnimation * animation = [CABasicAnimation animationWithKeyPath:@"strokeEnd"];
      animation.duration = 0.3;
      animation.repeatCount = 1;
      animation.fromValue = @(_progress / progress);
      animation.toValue = @1;
      animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
      animation.delegate = self;
      [self.progressLayer addAnimation:animation forKey:CircleAnimationKey];
    }
    else
    {
      self.progressLayer.path = path.CGPath;
    }
    _progress = progress;
  }
}

- (void)animationDidStop:(CAAnimation *)anim finished:(BOOL)flag
{
  if (self.nextProgressToAnimate)
    [self setProgress:[self.nextProgressToAnimate doubleValue] animated:YES];
}

- (void)setFailedMode:(BOOL)failedMode
{
  _failedMode = failedMode;

  self.progressLayer.strokeColor = (failedMode ? [UIColor redColor] : [UIColor linkBlue]).CGColor;
  self.stopRectLayer.hidden = failedMode;
  self.startTriangleView.hidden = !failedMode;
}

- (UIButton *)button
{
  if (!_button)
  {
    _button = [[UIButton alloc] initWithFrame:self.bounds];
    [_button addTarget:self action:@selector(buttonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _button;
}

- (CAShapeLayer *)stopRectLayer
{
  if (!_stopRectLayer)
  {
    _stopRectLayer = [CAShapeLayer layer];
    CGFloat const side = 11;
    CGRect const rect = CGRectMake((self.width - side) / 2, (self.width - side) / 2, side, side);
    _stopRectLayer.fillColor = [UIColor linkBlue].CGColor;
    _stopRectLayer.path = [UIBezierPath bezierPathWithRoundedRect:rect cornerRadius:2].CGPath;
  }
  return _stopRectLayer;
}

- (UIImageView *)startTriangleView
{
  if (!_startTriangleView)
    _startTriangleView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ProgressTriangle"]];
  return _startTriangleView;
}

- (CAShapeLayer *)backgroundCircleLayer
{
  if (!_backgroundCircleLayer)
  {
    _backgroundCircleLayer = [CAShapeLayer layer];
    _backgroundCircleLayer.fillColor = [UIColor clearColor].CGColor;
    _backgroundCircleLayer.lineWidth = 2;
    _backgroundCircleLayer.strokeColor = [UIColor pressBackground].CGColor;
    _backgroundCircleLayer.shouldRasterize = YES;
    _backgroundCircleLayer.rasterizationScale = 2 * [UIScreen mainScreen].scale;
    CGRect rect = CGRectMake(self.width / 2 - CIRCLE_RADIUS, self.height / 2 - CIRCLE_RADIUS, 2 * CIRCLE_RADIUS, 2 * CIRCLE_RADIUS);
    rect = CGRectInset(rect, _backgroundCircleLayer.lineWidth, _backgroundCircleLayer.lineWidth);
    _backgroundCircleLayer.path = [UIBezierPath bezierPathWithOvalInRect:rect].CGPath;
  }
  return _backgroundCircleLayer;
}

- (CAShapeLayer *)progressLayer
{
  if (!_progressLayer)
  {
    _progressLayer = [CAShapeLayer layer];
    _progressLayer.fillColor = [UIColor clearColor].CGColor;
    _progressLayer.lineWidth = 3;
    _progressLayer.shouldRasterize = YES;
    _progressLayer.rasterizationScale = 2 * [UIScreen mainScreen].scale;
  }
  return _progressLayer;
}

@end
