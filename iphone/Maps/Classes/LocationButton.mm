
#import "LocationButton.h"
#import "UIKitCategories.h"

@implementation LocationButton
{
  UIImageView * topImageView;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  self.contentMode = UIViewContentModeCenter;
  [super setImage:[UIImage imageNamed:@"LocationBackground"] forState:UIControlStateNormal];
  [self setImage:[UIImage imageNamed:@"LocationDefault"] forState:UIControlStateSelected];

  return self;
}

- (void)setImage:(UIImage *)image forState:(UIControlState)state
{
  [topImageView removeFromSuperview];
  topImageView = [[UIImageView alloc] initWithImage:image];
  [self addSubview:topImageView];
  topImageView.center = CGPointMake(self.width / 2, self.height / 2);
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
}

@end
