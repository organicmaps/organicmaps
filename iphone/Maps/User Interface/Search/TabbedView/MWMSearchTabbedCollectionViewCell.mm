#import "MWMSearchTabbedCollectionViewCell.h"
#import "MWMKeyboard.h"

@interface MWMSearchTabbedCollectionViewCell ()<MWMKeyboardObserver>

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * noResultsBottomOffset;
@property(weak, nonatomic) IBOutlet UIView * noResultsContainer;
@property(weak, nonatomic) IBOutlet UIView * noResultsWrapper;

@end

@implementation MWMSearchTabbedCollectionViewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
  [MWMKeyboard addObserver:self];
}

- (void)addNoResultsView:(MWMSearchNoResults *)view
{
  [self removeNoResultsView];
  self.noResultsContainer.hidden = NO;
  [self.noResultsWrapper addSubview:view];
}

- (void)removeNoResultsView
{
  self.noResultsContainer.hidden = YES;
  [self.noResultsWrapper.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation
{
  CGFloat const keyboardHeight = [MWMKeyboard keyboardHeight];
  if (keyboardHeight >= self.height)
    return;

  self.noResultsBottomOffset.constant = keyboardHeight;
  [self layoutIfNeeded];
}

- (void)onKeyboardWillAnimate { [self layoutIfNeeded]; }
@end
