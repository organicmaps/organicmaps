#import "MWMSearchTableView.h"
#import "MWMKeyboard.h"
#import "MWMSearchNoResults.h"

@interface MWMSearchTableView ()<MWMKeyboardObserver>

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * noResultsBottomOffset;

@property(weak, nonatomic) IBOutlet UIView * noResultsContainer;
@property(weak, nonatomic) IBOutlet UIView * noResultsWrapper;
@property(nonatomic) MWMSearchNoResults * noResultsView;

@end

@implementation MWMSearchTableView

- (void)awakeFromNib
{
  [super awakeFromNib];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
  [MWMKeyboard addObserver:self];
}

- (void)hideNoResultsView:(BOOL)hide
{
  if (hide)
  {
    self.noResultsContainer.hidden = YES;
    [self.noResultsView removeFromSuperview];
  }
  else
  {
    self.noResultsContainer.hidden = NO;
    [self.noResultsWrapper addSubview:self.noResultsView];
    [self onKeyboardAnimation];
  }
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation
{
  CGFloat const keyboardHeight = [MWMKeyboard keyboardHeight];
  if (keyboardHeight >= self.height)
    return;

  self.noResultsBottomOffset.constant = keyboardHeight;
  if (self.superview)
    [self layoutIfNeeded];
}

- (void)onKeyboardWillAnimate
{
  if (self.superview)
    [self layoutIfNeeded];
}

- (MWMSearchNoResults *)noResultsView
{
  if (!_noResultsView)
  {
    _noResultsView = [MWMSearchNoResults viewWithImage:[UIImage imageNamed:@"img_search_not_found"]
                                                 title:L(@"search_not_found")
                                                  text:L(@"search_not_found_query")];
  }
  return _noResultsView;
}

@end
