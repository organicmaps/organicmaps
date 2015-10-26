#import "Common.h"
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
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self setupImages];
  [self setupColors];
  [self setupAnimationLayers];
}

#pragma mark - Setup

- (void)setupImages
{
  self.images = [NSMutableDictionary dictionary];
  [self setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateNormal];
  [self setImage:[UIImage imageNamed:@"ic_download_press"] forState:MWMCircularProgressStateHighlighted];
  [self setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateSelected];
  [self setImage:[UIImage imageNamed:@"ic_download_press"] forState:MWMCircularProgressStateSelectedHighlighted];
  [self setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateProgress];
  [self setImage:[UIImage imageNamed:@"ic_close_spinner_press"] forState:MWMCircularProgressStateProgressHighlighted];
  [self setImage:[UIImage imageNamed:@"ic_download_error"] forState:MWMCircularProgressStateFailed];
  [self setImage:[UIImage imageNamed:@"ic_download_error_press"] forState:MWMCircularProgressStateFailedHighlighted];
  [self setImage:[UIImage imageNamed:@"ic_check"] forState:MWMCircularProgressStateCompleted];
}

- (void)setupColors
{
  self.colors = [NSMutableDictionary dictionary];
  UIColor * progressColor = [UIColor linkBlue];
  UIColor * clearColor = [UIColor clearColor];
  [self setColor:clearColor forState:MWMCircularProgressStateNormal];
  [self setColor:clearColor forState:MWMCircularProgressStateHighlighted];
  [self setColor:clearColor forState:MWMCircularProgressStateSelected];
  [self setColor:clearColor forState:MWMCircularProgressStateSelectedHighlighted];
  [self setColor:progressColor forState:MWMCircularProgressStateProgress];
  [self setColor:progressColor forState:MWMCircularProgressStateProgressHighlighted];
  [self setColor:clearColor forState:MWMCircularProgressStateFailed];
  [self setColor:clearColor forState:MWMCircularProgressStateFailedHighlighted];
  [self setColor:clearColor forState:MWMCircularProgressStateCompleted];
}

- (void)setupAnimationLayers
{
  self.backgroundLayer = [CAShapeLayer layer];
  self.progressLayer = [CAShapeLayer layer];

  [self refresh];
  [self.layer addSublayer:self.backgroundLayer];
  [self.layer addSublayer:self.progressLayer];
}

- (void)setImage:(nonnull UIImage *)image forState:(MWMCircularProgressState)state
{
  self.images[@(state)] = image;
  [self refresh];
}

- (void)setColor:(nonnull UIColor *)color forState:(MWMCircularProgressState)state
{
  self.colors[@(state)] = color;
  [self refresh];
}

#pragma mark - Progress

- (void)refresh
{
  self.backgroundLayer.fillColor = self.progressLayer.fillColor = UIColor.clearColor.CGColor;
  self.backgroundLayer.lineWidth = self.progressLayer.lineWidth = kLineWidth;
  self.backgroundLayer.strokeColor = self.backgroundColor;
  self.progressLayer.strokeColor = self.progressColor;
  CGRect rect = CGRectInset(self.bounds, kLineWidth, kLineWidth);
  self.backgroundLayer.path = [UIBezierPath bezierPathWithOvalInRect:rect].CGPath;
  UIImage * normalImage, * highlightedImage;
  switch (self.state)
  {
    case MWMCircularProgressStateNormal:
    case MWMCircularProgressStateHighlighted:
      normalImage = self.images[@(MWMCircularProgressStateNormal)];
      highlightedImage = self.images[@(MWMCircularProgressStateHighlighted)];
      break;
    case MWMCircularProgressStateSelected:
    case MWMCircularProgressStateSelectedHighlighted:
      normalImage = self.images[@(MWMCircularProgressStateSelected)];
      highlightedImage = self.images[@(MWMCircularProgressStateSelectedHighlighted)];
      break;
    case MWMCircularProgressStateProgress:
    case MWMCircularProgressStateProgressHighlighted:
      normalImage = self.images[@(MWMCircularProgressStateProgress)];
      highlightedImage = self.images[@(MWMCircularProgressStateProgressHighlighted)];
      break;
    case MWMCircularProgressStateFailed:
    case MWMCircularProgressStateFailedHighlighted:
      normalImage = self.images[@(MWMCircularProgressStateFailed)];
      highlightedImage = self.images[@(MWMCircularProgressStateFailedHighlighted)];
      break;
    case MWMCircularProgressStateCompleted:
      normalImage = self.images[@(MWMCircularProgressStateCompleted)];
      highlightedImage = self.images[@(MWMCircularProgressStateCompleted)];
      break;
  }
  [self.button setImage:normalImage forState:UIControlStateNormal];
  [self.button setImage:highlightedImage forState:UIControlStateHighlighted];
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
  dispatch_async(dispatch_get_main_queue(), ^
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
  });
}

- (void)stopSpinner
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    if (self.spinner.hidden)
      return;
    self.backgroundLayer.hidden = self.progressLayer.hidden = NO;
    self.spinner.hidden = YES;
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
  [self refresh];
}

- (CGColorRef)backgroundColor
{
  switch (self.state)
  {
    case MWMCircularProgressStateProgress:
    case MWMCircularProgressStateProgressHighlighted:
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
  BOOL const needRefresh = !CGRectEqualToRect(self.frame, frame);
  super.frame = frame;
  if (needRefresh)
    [self refresh];
}

- (BOOL)animating
{
  return [self.progressLayer animationForKey:kAnimationKey] != nil;
}

@end
