
#import "SmallCompassView.h"
#import "UIKitCategories.h"

@interface SmallCompassView ()

@property (nonatomic) UIImageView * arrow;

@end

@implementation SmallCompassView

- (id)init
{
  UIImage * image = [UIImage imageNamed:@"CompassBackground"];
  self = [super initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];

  UIImageView * background = [[UIImageView alloc] initWithImage:image];
  [self addSubview:background];

  self.arrow = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"CompassArrow"]];
  [self addSubview:self.arrow];
  self.arrow.center = CGPointMake(self.width / 2, self.height / 2);

  return self;
}

- (void)setAngle:(CGFloat)angle
{
  self.arrow.transform = CGAffineTransformIdentity;
  self.arrow.transform = CGAffineTransformMakeRotation(angle);
  _angle = angle;
}

@end
