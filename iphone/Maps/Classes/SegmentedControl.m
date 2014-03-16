
#import "SegmentedControl.h"
#import "UIKitCategories.h"

@interface SegmentedControl ()

@property (nonatomic, readonly) UIButton * selectedButton;
@property (nonatomic) UIImageView * selectionImageView;

@property (nonatomic) UIButton * leftButton;
@property (nonatomic) UIButton * centerButton;
@property (nonatomic) UIButton * rightButton;

@end

@implementation SegmentedControl
@synthesize active = _active;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.selectionImageView];

  self.leftButton = [self buttonWithTitle:NSLocalizedString(@"search_mode_nearme", nil)];
  self.leftButton.tag = 0;
  self.centerButton = [self buttonWithTitle:NSLocalizedString(@"search_mode_viewport", nil)];
  self.centerButton.tag = 1;
  self.rightButton = [self buttonWithTitle:NSLocalizedString(@"search_mode_all", nil)];
  self.rightButton.tag = 2;

  [self setSelectedButton:self.leftButton animated:NO];
  [self setActive:NO animated:NO];

  return self;
}

- (NSInteger)segmentsCount
{
  return 3;
}

- (UIButton *)buttonWithTitle:(NSString *)title
{
  UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 90, 44)];
  [button setTitle:[title lowercaseString] forState:UIControlStateNormal];
  [button setTitleColor:[UIColor colorWithColorCode:@"333333"] forState:UIControlStateNormal];
  [button setTitleColor:[UIColor grayColor] forState:UIControlStateHighlighted];
  [button setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
  [button addTarget:self action:@selector(buttonPressed:) forControlEvents:UIControlEventTouchUpInside];
  button.midY = self.height / 2;
  button.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;;
  button.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:14];
  [self addSubview:button];

  return button;
}

- (UIImageView *)selectionImageView
{
  if (!_selectionImageView)
  {
    _selectionImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"SegmentSelection"]];
    _selectionImageView.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _selectionImageView;
}

- (void)setActive:(BOOL)active animated:(BOOL)animated
{
  if (active)
  {
    [UIView animateWithDuration:(animated ? 0.35 : 0) delay:0 damping:0.8 initialVelocity:1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      NSInteger count = 3;
      CGFloat start = (self.width / count) / 2;
      CGFloat delta = self.width / count;
      self.leftButton.midX = start;
      self.centerButton.midX = start + delta;
      self.rightButton.midX = start + 2 * delta;
      self.selectionImageView.center = self.selectedButton.center;
      self.leftButton.alpha = 1;
      self.centerButton.alpha = 1;
      self.rightButton.alpha = 1;
      self.selectionImageView.alpha = 1;
    } completion:^(BOOL finished){}];
  }
  else
  {
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:0.8 initialVelocity:1 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.leftButton.maxX = 0;
      self.rightButton.minX = self.width;
      self.centerButton.midX = self.width / 2;
      self.selectionImageView.center = self.selectedButton.center;
      self.leftButton.alpha = 0;
      self.centerButton.alpha = 0;
      self.rightButton.alpha = 0;
      self.selectionImageView.alpha = 0;
    } completion:^(BOOL finished){}];
  }
  _active = active;
}

- (void)layoutSubviews
{
  self.selectionImageView.center = self.selectedButton.center;
}

- (NSInteger)selectedSegmentIndex
{
  return self.selectedButton.tag;
}

- (void)setSelectedSegmentIndex:(NSInteger)selectedSegmentIndex
{
  if (selectedSegmentIndex == 0)
    [self setSelectedButton:self.leftButton animated:NO];
  else if (selectedSegmentIndex == 1)
    [self setSelectedButton:self.centerButton animated:NO];
  else if (selectedSegmentIndex == 2)
    [self setSelectedButton:self.rightButton animated:NO];
}

- (void)setSelectedButton:(UIButton *)selectedButton animated:(BOOL)animated
{
  _selectedButton.selected = NO;
  _selectedButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:14];
  _selectedButton.userInteractionEnabled = YES;
  selectedButton.selected = YES;
  selectedButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:14];
  selectedButton.userInteractionEnabled = NO;
  [UIView animateWithDuration:(animated ? 0.3 : 0) delay:0 damping:0.8 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.selectionImageView.center = selectedButton.center;
  } completion:^(BOOL finished){}];

  _selectedButton = selectedButton;
}

- (void)buttonPressed:(UIButton *)sender
{
  [self setSelectedButton:sender animated:YES];
  [self.delegate segmentedControl:self didSelectSegment:sender.tag];
}

@end
