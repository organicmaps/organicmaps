#import "MWMRouteTypeButton.h"

@interface MWMRouteTypeButton ()

@property (nonatomic, weak) IBOutlet UIButton * button;
@property (nonatomic, weak) IBOutlet UIImageView * spinner;

@end

@implementation MWMRouteTypeButton

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  if (self = [super initWithCoder:aDecoder])
  {
    UIView * v = [[NSBundle.mainBundle loadNibNamed:self.class.className owner:self options:nil] firstObject];
    [self addSubview:v];
    [self.button addTarget:self action:@selector(tap) forControlEvents:UIControlEventTouchUpInside];
    self.spinner.hidden = YES;
  }
  return self;
}

- (void)stopAnimating
{
  self.spinner.hidden = YES;
  [self.spinner stopAnimating];
}

- (void)startAnimating
{
  self.spinner.hidden = NO;
  NSUInteger const animationImagesCount = 12;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"Spinner_%@", @(i+1)]];

  self.spinner.animationImages = animationImages;
  [self.spinner startAnimating];
}

- (void)setIcon:(UIImage *)icon
{
  _icon = icon;
  [self.button setImage:icon forState:UIControlStateNormal];
}

- (void)setHighlightedIcon:(UIImage *)highlightedIcon
{
  _highlightedIcon = highlightedIcon;
  [self.button setImage:highlightedIcon forState:UIControlStateHighlighted];
}

- (void)setSelectedIcon:(UIImage *)selectedIcon
{
  _selectedIcon = selectedIcon;
  [self.button setImage:selectedIcon forState:UIControlStateSelected];
}

- (void)setSelected:(BOOL)selected
{
  self.button.selected = selected;
  if (!selected)
    [self stopAnimating];
}

- (BOOL)isSelected
{
  return self.button.isSelected;
}

- (void)tap
{
  if (self.selected)
    return;

  [self startAnimating];
  [self sendActionsForControlEvents:UIControlEventTouchUpInside];
}

@end
