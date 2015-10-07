#import "Common.h"
#import "MWMNavigationView.h"

static CGFloat const kStatusbarHeight = 20.0;

@interface MWMNavigationView ()

@property (nonatomic) BOOL isVisible;
@property (nonatomic, readwrite) CGFloat defaultHeight;

@property (weak, nonatomic, readwrite) IBOutlet UIView * contentView;

@end

@implementation MWMNavigationView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.statusbarBackground = [[UIView alloc] initWithFrame:CGRectZero];
  self.statusbarBackground.backgroundColor = self.contentView.backgroundColor;
  self.defaultHeight = self.height;
  self.topBound = 0.0;
  self.leftBound = 0.0;
}

- (void)addToView:(UIView *)superview
{
  NSAssert(superview != nil, @"Superview can't be nil");
  if ([superview.subviews containsObject:self])
    return;
  dispatch_async(dispatch_get_main_queue(), ^
  {
    [superview insertSubview:self atIndex:0];
    self.frame = self.defaultFrame;
    self.isVisible = YES;
  });
}

- (void)remove
{
  self.isVisible = NO;
}

- (void)layoutSubviews
{
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    if (!CGRectEqualToRect(self.frame, self.defaultFrame))
      self.frame = self.defaultFrame;
    self.statusbarBackground.frame = CGRectMake(0.0, -kStatusbarHeight, self.width, kStatusbarHeight);
    [self.delegate navigationDashBoardDidUpdate];
  }
  completion:^(BOOL finished)
  {
    if (!self.isVisible)
      [self removeFromSuperview];
  }];
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  return CGRectMake(self.leftBound, self.isVisible ? self.topBound : -self.defaultHeight,
                    self.superview.width, self.defaultHeight);
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = MAX(topBound, kStatusbarHeight);
  if (_topBound <= kStatusbarHeight)
  {
    if (![self.statusbarBackground.superview isEqual:self])
      [self addSubview:self.statusbarBackground];
  }
  else
  {
    [self.statusbarBackground removeFromSuperview];
  }
  [self setNeedsLayout];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = leftBound;
  [self setNeedsLayout];
}

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = isVisible;
  [self setNeedsLayout];
}

- (CGFloat)visibleHeight
{
  CGFloat height = self.contentView.height;
  if ([self.statusbarBackground.superview isEqual:self])
    height += self.statusbarBackground.height;
  return height;
}

@end
