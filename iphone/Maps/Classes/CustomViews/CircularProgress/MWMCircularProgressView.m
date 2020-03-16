#import "MWMCircularProgressView.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"

static CGFloat const kLineWidth = 2.0;
static NSString * const kAnimationKey = @"CircleAnimation";

static CGFloat angleWithProgress(CGFloat progress) { return 2.0 * M_PI * progress - M_PI_2; }

@interface MWMCircularProgressView ()

@property(nonatomic) CAShapeLayer * backgroundLayer;
@property(nonatomic) CAShapeLayer * progressLayer;

@property(nonatomic) UIColor * spinnerBackgroundColor;
@property(nonatomic, readonly) CGColorRef progressLayerColor;

@property(nonatomic) NSMutableDictionary * colors;
@property(nonatomic) NSMutableDictionary<NSNumber *, NSNumber *> * buttonColoring;
@property(nonatomic) NSMutableDictionary<NSNumber *, NSString *> * images;

@property(weak, nonatomic) IBOutlet MWMCircularProgress * owner;
@property(weak, nonatomic) IBOutlet UIImageView * spinner;
@property(weak, nonatomic) IBOutlet MWMButton * button;

@property(nonatomic) BOOL suspendRefreshProgress;

@end

@implementation MWMCircularProgressView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.suspendRefreshProgress = YES;
  [self setupColors];
  [self setupButtonColoring];
  self.images = [NSMutableDictionary dictionary];
  [self setupAnimationLayers];
  self.suspendRefreshProgress = NO;
}

#pragma mark - Setup

- (void)setupColors
{
  self.colors = [NSMutableDictionary dictionary];
  UIColor * progressColor = [_spinnerBackgroundColor isEqual:UIColor.clearColor]
                                ? UIColor.whiteColor
                                : [UIColor linkBlue];
  UIColor * clearColor = UIColor.clearColor;
  [self setSpinnerColoring:MWMImageColoringGray];
  [self setColor:clearColor forState:MWMCircularProgressStateNormal];
  [self setColor:clearColor forState:MWMCircularProgressStateSelected];
  [self setColor:progressColor forState:MWMCircularProgressStateProgress];
  [self setColor:progressColor forState:MWMCircularProgressStateSpinner];
  [self setColor:clearColor forState:MWMCircularProgressStateFailed];
  [self setColor:clearColor forState:MWMCircularProgressStateCompleted];
}

- (void)setupButtonColoring
{
  self.buttonColoring = [NSMutableDictionary dictionary];
  [self setColoring:MWMButtonColoringBlack forState:MWMCircularProgressStateNormal];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateSelected];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateProgress];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateSpinner];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateFailed];
  [self setColoring:MWMButtonColoringBlue forState:MWMCircularProgressStateCompleted];
}

- (void)applyTheme
{
  [super applyTheme];
  self.suspendRefreshProgress = YES;
  [self setupColors];
  self.suspendRefreshProgress = NO;
}

- (void)setupAnimationLayers
{
  self.backgroundLayer = [CAShapeLayer layer];
  self.progressLayer = [CAShapeLayer layer];

  [self refreshProgress];
  [self.layer addSublayer:self.backgroundLayer];
  [self.layer addSublayer:self.progressLayer];
}

- (void)setSpinnerColoring:(MWMImageColoring)coloring { self.spinner.mwm_coloring = coloring; }
- (void)setImageName:(nullable NSString *)imageName forState:(MWMCircularProgressState)state
{
  self.images[@(state)] = imageName;
  [self refreshProgress];
}

- (void)setColor:(UIColor *)color forState:(MWMCircularProgressState)state
{
  self.colors[@(state)] = color;
  [self refreshProgress];
}

- (void)setColoring:(MWMButtonColoring)coloring forState:(MWMCircularProgressState)state
{
  self.buttonColoring[@(state)] = @(coloring);
  [self refreshProgress];
}

#pragma mark - Progress

- (void)refreshProgress
{
  if (self.suspendRefreshProgress)
    return;
  self.backgroundLayer.fillColor = self.progressLayer.fillColor = UIColor.clearColor.CGColor;
  self.backgroundLayer.lineWidth = self.progressLayer.lineWidth = kLineWidth;
  self.backgroundLayer.strokeColor = self.spinnerBackgroundColor.CGColor;
  self.progressLayer.strokeColor = self.progressLayerColor;
  CGPoint center = CGPointMake(self.width / 2.0, self.height / 2.0);
  CGFloat radius = MIN(center.x, center.y) - kLineWidth;
  UIBezierPath * path = [UIBezierPath bezierPathWithArcCenter:center
                                                       radius:radius
                                                   startAngle:angleWithProgress(0.0)
                                                     endAngle:angleWithProgress(1.0)
                                                    clockwise:YES];
  self.backgroundLayer.path = path.CGPath;
  NSString * imageName = self.images[@(self.state)];
  if (imageName)
  {
    [self.button setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
    UIImage *hl = [UIImage imageNamed:[imageName stringByAppendingString:@"_highlighted"]];
    if (hl)
      [self.button setImage:hl forState:UIControlStateHighlighted];
  }
  else
  {
    [self.button setImage:nil forState:UIControlStateNormal];
    [self.button setImage:nil forState:UIControlStateHighlighted];
  }

  self.button.coloring = (MWMButtonColoring)self.buttonColoring[@(self.state)].unsignedIntegerValue;
}

- (void)updatePath:(CGFloat)progress
{
  if (progress > 0.0)
  {
    self.state =
        progress < 1.0 ? MWMCircularProgressStateProgress : MWMCircularProgressStateCompleted;
    [self stopSpinner];
  }
  CGPoint center = CGPointMake(self.width / 2.0, self.height / 2.0);
  CGFloat radius = MIN(center.x, center.y) - kLineWidth;
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
  NSString * postfix = ([UIColor isNightMode] && !self.isInvertColor) ||
                               (![UIColor isNightMode] && self.isInvertColor) ||
                               _spinnerBackgroundColor
                           ? @"dark"
                           : @"light";
  UIImage * image = [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@", postfix]];
  self.spinner.image = image;
  [self.spinner startRotation:1];
}

- (void)stopSpinner
{
  if (self.spinner.hidden)
    return;
  self.spinner.hidden = YES;
  self.backgroundLayer.hidden = self.progressLayer.hidden = NO;
  [self.spinner stopRotation];
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
  animation.timingFunction =
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
  animation.delegate = self.owner;
  [self.progressLayer addAnimation:animation forKey:kAnimationKey];
}

#pragma mark - Properties

- (UIView * _Nullable)buttonView
{
  return self.button;
}

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

- (UIColor *)spinnerBackgroundColor
{
  if (_spinnerBackgroundColor)
    return _spinnerBackgroundColor;
  switch (self.state)
  {
  case MWMCircularProgressStateProgress: return [UIColor pressBackground];
  default: return UIColor.clearColor;
  }
}

- (CGColorRef)progressLayerColor
{
  UIColor * color = self.colors[@(self.state)];
  return color.CGColor;
}

- (void)setFrame:(CGRect)frame
{
  BOOL needrefreshProgress = !CGRectEqualToRect(self.frame, frame);
  super.frame = frame;
  if (needrefreshProgress)
    [self refreshProgress];
}

- (BOOL)animating { return [self.progressLayer animationForKey:kAnimationKey] != nil; }
- (void)setSuspendRefreshProgress:(BOOL)suspendRefreshProgress
{
  _suspendRefreshProgress = suspendRefreshProgress;
  if (!suspendRefreshProgress)
    [self refreshProgress];
}

@end
