
#import "LocationButton.h"
#import "UIKitCategories.h"

@implementation LocationButton
{
  UIImageView * topImageView;
  BOOL searching;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  self.contentMode = UIViewContentModeCenter;
  [self setImage:[UIImage imageNamed:@"LocationDefault"] forState:UIControlStateSelected];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(willEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];

  return self;
}

- (void)willEnterForeground:(NSNotification *)notification
{
  if (searching)
    [self setSearching];
}

- (void)setImage:(UIImage *)image forState:(UIControlState)state
{
  [topImageView removeFromSuperview];
  topImageView = [[UIImageView alloc] initWithImage:image];
  [self addSubview:topImageView];
  topImageView.center = CGPointMake(self.width / 2, self.height / 2);
  searching = NO;
}

- (void)setSearching
{
  CABasicAnimation * animation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
  CGFloat count = CGFLOAT_MAX;
  animation.toValue = @(2 * M_PI * count);
  NSTimeInterval loopDuration = 1.2;
  animation.duration = count * loopDuration;
  animation.repeatCount = count;
  [topImageView.layer addAnimation:animation forKey:@"rotation"];
  searching = YES;
}

@end
