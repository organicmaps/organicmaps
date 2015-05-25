//
//#import "ContainerView.h"
//#import "UIKitCategories.h"
//
//@interface ContainerView ()
//
//@property (nonatomic) SolidTouchView * fadeView;
//@property (nonatomic) SolidTouchView * swipeView;
//
//@end
//
//
//@implementation ContainerView
//
//- (id)initWithFrame:(CGRect)frame
//{
//  self = [super initWithFrame:frame];
//
//  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
//  self.clipsToBounds = YES;
//
//  [self addSubview:self.fadeView];
//  [self addSubview:self.placePage];
//  [self addSubview:self.swipeView];
//  self.fadeView.alpha = 0;
//  self.swipeView.userInteractionEnabled = NO;
//
//  return self;
//}
//
////- (void)swipe:(UISwipeGestureRecognizer *)sender
////{
////  if (self.placePage.state == PlacePageStateOpened)
////    [self.placePage setState:PlacePageStateHidden animated:YES withCallback:YES];
////}
//
//- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
//{
//  return (self.placePage.state == PlacePageStateOpened) ? YES : CGRectIntersectsRect(self.placePage.frame, CGRectMake(point.x, point.y, 0, 0));
//}
//
//- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
//{
//  if (object == self.placePage && [keyPath isEqualToString:@"state"])
//  {
//    self.swipeView.frame = CGRectMake(0, self.placePage.maxY, self.width, self.height - self.placePage.height);
//    self.swipeView.userInteractionEnabled = (self.placePage.state == PlacePageStateOpened) ? YES : NO;
//    [UIView animateWithDuration:0.4 delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
//      self.fadeView.alpha = (self.placePage.state == PlacePageStateOpened) ? 1 : 0;
//    } completion:^(BOOL finished) {}];
//  }
//}
//
//- (void)tap:(UITapGestureRecognizer *)sender
//{
//  [self.placePage setState:PlacePageStateHidden animated:YES withCallback:YES];
//}
//
//- (PlacePageView *)placePage
//{
//  if (!_placePage)
//  {
//    _placePage = [[PlacePageView alloc] initWithFrame:self.bounds];
//    [_placePage addObserver:self forKeyPath:@"state" options:NSKeyValueObservingOptionNew context:nil];
//  }
//  return _placePage;
//}
//
//- (SolidTouchView *)fadeView
//{
//  if (!_fadeView)
//  {
//    _fadeView = [[SolidTouchView alloc] initWithFrame:self.bounds];
//    _fadeView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.5];
//    _fadeView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
//  }
//  return _fadeView;
//}
//
//- (SolidTouchView *)swipeView
//{
//  if (!_swipeView)
//  {
//    _swipeView = [[SolidTouchView alloc] initWithFrame:CGRectZero];
//
//    UISwipeGestureRecognizer * swipe = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
//    [_swipeView addGestureRecognizer:swipe];
//    swipe.direction = UISwipeGestureRecognizerDirectionUp;
//
//    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
//    [_swipeView addGestureRecognizer:tap];
//  }
//  return _swipeView;
//}
//
//@end
