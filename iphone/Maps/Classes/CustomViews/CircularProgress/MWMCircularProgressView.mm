#import "Common.h"
#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"
#import "UIButton+Coloring.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"

static CGFloat const kLineWidth = 2.0;
static NSString * const kAnimationKey = @"CircleAnimation";

static inline CGFloat angleWithProgress(CGFloat progress)
{
  return 2.0 * M_PI * progress - M_PI_2;
}

@interface MWMCircularProgressView ()

@property (nonatomic) CAShapeLayer * backgroundLayer;
@property (nonatomic) CAShapeLayer * progressLayer;

@property (nonatomic, readonly) CGColorRef backgroundColor;
@property (nonatomic, readonly) CGColorRef progressColor;

@property (nonatomic) NSMutableDictionary * images;
@property (nonatomic) NSMutableDictionary * colors;

@property (weak, nonatomic) IBOutlet MWMCircularProgress * owner;
@property (weak, nonatomic) IBOutlet UIImageView * spinner;
@property (weak, nonatomic) IBOutlet UIButton * button;

@end

@implementation MWMCircularProgressView

- (void)awakeFromNib
{
  self.images = [NSMutableDictionary dictionary];
  [self setupColors];
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
  [self setColor:clearColor forState:MWMCircularProgressStateFailed];
  [self setColor:clearColor forState:MWMCircularProgressStateCompleted];
}

- (void)refresh
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

#pragma mark - Progress

- (void)refreshProgress
{
  self.backgroundLayer.fillColor = self.progressLayer.fillColor = UIColor.clearColor.CGColor;
  self.backgroundLayer.lineWidth = self.progressLayer.lineWidth = kLineWidth;
  self.backgroundLayer.strokeColor = self.backgroundColor;
  self.progressLayer.strokeColor = self.progressColor;
  CGRect rect = CGRectInset(self.bounds, kLineWidth, kLineWidth);
  self.backgroundLayer.path = [UIBezierPath bezierPathWithOvalInRect:rect].CGPath;
  UIImage * normalImage = nil;
  switch (self.state)
  {
    case MWMCircularProgressStateNormal:
      normalImage = self.images[@(MWMCircularProgressStateNormal)];
      self.button.mwm_coloring = MWMButtonColoringBlack;
      break;
    case MWMCircularProgressStateSelected:
      normalImage = self.images[@(MWMCircularProgressStateSelected)];
      self.button.mwm_coloring = MWMButtonColoringBlue;
      break;
    case MWMCircularProgressStateProgress:
      normalImage = self.images[@(MWMCircularProgressStateProgress)];
      self.button.mwm_coloring = MWMButtonColoringBlue;
      break;
    case MWMCircularProgressStateFailed:
      normalImage = self.images[@(MWMCircularProgressStateFailed)];
      self.button.mwm_coloring = MWMButtonColoringBlue;
      break;
    case MWMCircularProgressStateCompleted:
      normalImage = self.images[@(MWMCircularProgressStateCompleted)];
      self.button.mwm_coloring = MWMButtonColoringBlue;
      break;
  }
  [self.button setImage:normalImage forState:UIControlStateNormal];
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
  if (!self.spinner.hidden)
    return;
  self.spinner.hidden = NO;
  dispatch_async(dispatch_get_main_queue(), ^
  {
    self.backgroundLayer.hidden = self.progressLayer.hidden = YES;
    NSUInteger const animationImagesCount = 12;
    NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
    for (NSUInteger i = 0; i < animationImagesCount; ++i)
      animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@_%@", @(i+1),
                                                [UIColor isNightMode] ? @"dark" : @"light"]];

    self.spinner.animationImages = animationImages;
    [self.spinner startAnimating];
  });
}

- (void)stopSpinner
{
  if (self.spinner.hidden)
    return;
  self.spinner.hidden = YES;
  dispatch_async(dispatch_get_main_queue(), ^
  {
    self.backgroundLayer.hidden = self.progressLayer.hidden = NO;
    [self.spinner.layer removeAllAnimations];
  });
}

#pragma mark - Animation

- (void)animateFromValue:(CGFloat)fromValue toValue:(CGFloat)toValue
{
  dispatch_async(dispatch_get_main_queue(), ^
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
  });
}

#pragma mark - Properties

- (void)setState:(MWMCircularProgressState)state
{
  if (_state == state)
    return;
  _state = state;
  [self refreshProgress];
}

- (CGColorRef)backgroundColor
{
  switch (self.state)
  {
    case MWMCircularProgressStateProgress:
      return [UIColor pressBackground].CGColor;
    default:
      return [UIColor clearColor].CGColor;
  }
}

- (CGColorRef)progressColor
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
