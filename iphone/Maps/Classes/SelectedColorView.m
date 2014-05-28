
#import "SelectedColorView.h"
#import "CircleView.h"
#import "UIKitCategories.h"

@interface SelectedColorView ()

@property (nonatomic) UIButton * button;
@property (nonatomic) UIImageView * circleView;

@end

@implementation SelectedColorView

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.button];
  [self addSubview:self.circleView];
  self.circleView.center = CGPointMake(self.width / 2, self.height / 2);

  return self;
}

- (void)buttonPressed:(UIButton *)sender
{
  [self.delegate selectedColorViewDidPress:self];
}

- (void)setColor:(UIColor *)color
{
  self.circleView.image = [CircleView createCircleImageWith:self.circleView.width andColor:color];
}

- (UIImageView *)circleView
{
  if (!_circleView)
    _circleView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 15, 15)];
  return _circleView;
}

- (UIButton *)button
{
  if (!_button)
  {
    _button = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 44, 44)];
    _button.contentMode = UIViewContentModeCenter;
    [_button addTarget:self action:@selector(buttonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_button setImage:[UIImage imageNamed:@"ColorSelectorBackground"] forState:UIControlStateNormal];
  }
  return _button;
}

@end
