#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"
#import "UIColor+MapsMeColor.h"

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
@property (weak, nonatomic) IBOutlet UIImageView * spinner;

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
  self.backgroundLayer.fillColor = UIColor.clearColor.CGColor;
  self.backgroundLayer.lineWidth = kLineWidth;
  self.backgroundLayer.strokeColor = UIColor.pressBackground.CGColor;
  CGRect rect = CGRectInset(self.bounds, kLineWidth, kLineWidth);
  self.backgroundLayer.path = [UIBezierPath bezierPathWithOvalInRect:rect].CGPath;
}

- (void)refreshProgress
{
  self.progressLayer.fillColor = UIColor.clearColor.CGColor;
  self.progressLayer.lineWidth = kLineWidth;
  self.progressLayer.strokeColor = self.owner.failed ? UIColor.red.CGColor : UIColor.linkBlue.CGColor;
}

- (void)updatePath:(CGFloat)progress
{
  if (progress > 0.0)
    [self stopSpinner];
  CGFloat const outerRadius = self.width / 2.0;
  CGPoint const center = CGPointMake(outerRadius, outerRadius);
  CGFloat const radius = outerRadius - kLineWidth;
  UIBezierPath * path = [UIBezierPath bezierPathWithArcCenter:center radius:radius startAngle:angleWithProgress(0.0) endAngle:angleWithProgress(progress) clockwise:YES];
  self.progressLayer.path = path.CGPath;
}

#pragma mark - Spinner

- (void)startSpinner
{
  if (!self.spinner.hidden)
    return;
  self.backgroundLayer.hidden = self.progressLayer.hidden = YES;
  self.spinner.hidden = NO;
  NSUInteger const animationImagesCount = 12;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@", @(i+1)]];

  self.spinner.animationImages = animationImages;
  [self.spinner startAnimating];
}

- (void)stopSpinner
{
  if (self.spinner.hidden)
    return;
  self.backgroundLayer.hidden = self.progressLayer.hidden = NO;
  self.spinner.hidden = YES;
  [self.spinner.layer removeAllAnimations];
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
