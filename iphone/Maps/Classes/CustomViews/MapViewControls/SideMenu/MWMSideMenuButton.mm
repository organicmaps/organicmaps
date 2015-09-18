#import "MWMMapViewControlsCommon.h"
#import "MWMSideMenuButton.h"

@interface MWMSideMenuButton()

@property (weak, nonatomic) IBOutlet UIImageView * buttonImage;
@property (nonatomic) CGRect defaultBounds;

@end

@implementation MWMSideMenuButton

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    self.defaultBounds = self.bounds;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return self;
}

- (void)setup
{
  self.contentScaleFactor = self.superview.contentScaleFactor;
  self.alpha = 0.0;
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.alpha = 1.0;
  }];
  UIImageView const * const animationIV = self.buttonImage;
  NSString const * const imageName = @"btn_green_menu_";
  self.buttonImage.image = [UIImage imageNamed:[imageName stringByAppendingString:@"1"]];
  static NSUInteger const animationImagesCount = 4;
  NSMutableArray * const animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"%@%@", imageName, @(animationImagesCount - i)]];
  animationIV.animationImages = animationImages;
  animationIV.animationDuration = framesDuration(animationIV.animationImages.count);
  animationIV.animationRepeatCount = 1;
  [animationIV startAnimating];
  [self setNeedsLayout];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.buttonImage.frame = self.bounds = self.defaultBounds;
  self.maxY = self.superview.height - 2.0 * kViewControlsOffsetToBounds;
  [self layoutXPosition:self.hidden];
  [self.delegate menuButtonDidUpdateLayout];
}

- (void)layoutXPosition:(BOOL)hidden
{
  if (hidden)
  {
    self.minX = self.superview.width;
  }
  else
  {
    self.maxX = self.superview.width - 2.0 * kViewControlsOffsetToBounds;

    m2::PointD const pivot(self.minX * self.contentScaleFactor - 2.0 * kViewControlsOffsetToBounds, self.maxY * self.contentScaleFactor - kViewControlsOffsetToBounds);
    [self.delegate setRulerPivot:pivot];
    [self.delegate setCopyrightLabelPivot:pivot];
  }
}

- (void)handleSingleTap
{
  [self.delegate handleSingleTap];
}

- (void)handleDoubleTap
{
  [self.delegate handleDoubleTap];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  self.buttonImage.highlighted = YES;
  UITouch * touch = [touches anyObject];
  if (touch.tapCount > 1)
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  self.buttonImage.highlighted = NO;
  UITouch * touch = [touches anyObject];
  if (touch.tapCount == 1)
    [self performSelector:@selector(handleSingleTap) withObject:nil afterDelay:0.1];
  else if (touch.tapCount > 1)
    [self handleDoubleTap];
}

#pragma mark - Properties

- (void)setHidden:(BOOL)hidden animated:(BOOL)animated
{
  if (self.hidden == hidden)
    return;
  if (animated)
  {
    if (!hidden)
      self.hidden = NO;
    [self layoutXPosition:!hidden];
    [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
    {
      [self layoutXPosition:hidden];
    }
    completion:^(BOOL finished)
    {
      if (hidden)
        self.hidden = YES;
    }];
  }
  else
  {
    self.hidden = hidden;
    [self layoutXPosition:hidden];
  }
}

#pragma mark - Properties

- (void)setDownloadBadge:(MWMSideMenuDownloadBadge *)downloadBadge
{
  _downloadBadge = downloadBadge;
  if (![downloadBadge.superview isEqual:self])
    [self addSubview:downloadBadge];
}

- (CGRect)frameWithSpacing
{
  CGFloat const outset = 2.0 * kViewControlsOffsetToBounds;
  return CGRectInset(self.frame, -outset, -outset);
}

@end
