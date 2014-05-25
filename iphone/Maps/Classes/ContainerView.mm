
#import "ContainerView.h"
#import "UIKitCategories.h"

@interface ContainerView ()

@property (nonatomic) UIView * fadeView;
@property (nonatomic) UIView * swipeView;

@end


@implementation ContainerView

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.clipsToBounds = YES;

  [self addSubview:self.fadeView];
  [self addSubview:self.placePage];
  [self addSubview:self.swipeView];
  self.fadeView.alpha = 0;
  self.swipeView.userInteractionEnabled = NO;

  return self;
}

- (void)swipe:(UISwipeGestureRecognizer *)sender
{
  if (self.placePage.state == PlacePageStateOpened)
    [self.placePage setState:PlacePageStatePreview animated:YES withCallback:YES];
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
  return (self.placePage.state == PlacePageStateOpened) ? YES : CGRectIntersectsRect(self.placePage.frame, CGRectMake(point.x, point.y, 0, 0));
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if (object == self.placePage && [keyPath isEqualToString:@"state"])
  {
    CGFloat const panViewShift = 12;
    self.swipeView.frame = CGRectMake(0, self.placePage.maxY - panViewShift, self.width, self.height - self.placePage.height - panViewShift);
    self.swipeView.userInteractionEnabled = (self.placePage.state == PlacePageStateOpened) ? YES : NO;
    [UIView animateWithDuration:0.4 delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.fadeView.alpha = (self.placePage.state == PlacePageStateOpened) ? 1 : 0;
    } completion:^(BOOL finished) {}];
  }
}

- (void)tap:(UITapGestureRecognizer *)sender
{
  [self.placePage setState:PlacePageStatePreview animated:YES withCallback:YES];
}

- (PlacePageView *)placePage
{
  if (!_placePage)
  {
    _placePage = [[PlacePageView alloc] initWithFrame:self.bounds];
    [_placePage addObserver:self forKeyPath:@"state" options:NSKeyValueObservingOptionNew context:nil];
  }
  return _placePage;
}

- (UIView *)fadeView
{
  if (!_fadeView)
  {
    _fadeView = [[UIView alloc] initWithFrame:self.bounds];
    _fadeView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.5];
    _fadeView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  }
  return _fadeView;
}

- (UIView *)swipeView
{
  if (!_swipeView)
  {
    _swipeView = [[UIView alloc] initWithFrame:CGRectZero];
    _swipeView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    _swipeView.backgroundColor = [[UIColor redColor] colorWithAlphaComponent:0.3];

    UISwipeGestureRecognizer * swipe = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
    [_swipeView addGestureRecognizer:swipe];
    swipe.direction = UISwipeGestureRecognizerDirectionUp;

    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
    [_swipeView addGestureRecognizer:tap];
  }
  return _swipeView;
}

@end
