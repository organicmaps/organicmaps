#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMBottomMenuView.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

#include "Framework.h"

@interface MWMBottomMenuView ()

@property(weak, nonatomic) IBOutlet UIView * mainButtons;
@property(weak, nonatomic) IBOutlet UIView * separator;
@property(weak, nonatomic) IBOutlet UICollectionView * additionalButtons;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * mainButtonWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * separatorWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * additionalButtonsWidth;

@property(weak, nonatomic) IBOutlet UIView * downloadBadge;

@property(weak, nonatomic) IBOutlet UIButton * locationButton;
@property(weak, nonatomic) IBOutlet UIButton * p2pButton;
@property(weak, nonatomic) IBOutlet UIButton * searchButton;
@property(weak, nonatomic) IBOutlet UIButton * bookmarksButton;
@property(weak, nonatomic) IBOutlet UIButton * menuButton;

@property(weak, nonatomic) IBOutlet UIButton * goButton;

@property(weak, nonatomic) IBOutlet UILabel * streetLabel;

@property(nonatomic) CGFloat layoutDuration;

@end

@implementation MWMBottomMenuView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.additionalButtons.hidden = self.goButton.hidden = self.streetLabel.hidden = self.downloadBadge.hidden = YES;
  self.restoreState = MWMBottomMenuStateInactive;
}

- (void)layoutSubviews
{
  [self refreshOnOrientationChange];
  if (self.layoutDuration > 0.0)
  {
    CGFloat const duration = self.layoutDuration;
    self.layoutDuration = 0.0;
    [self layoutIfNeeded];
    [UIView animateWithDuration:duration
                     animations:^
    {
      [self layoutGeometry];
      [self layoutIfNeeded];
    }];
  }
  else
  {
    [self layoutGeometry];
  }
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self updateAlphaAndColor]; }
                   completion:^(BOOL finished) { [self updateVisibility]; }];
  [self layoutWidgets];
  [super layoutSubviews];
}

- (void)updateAlphaAndColor
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden:
    break;
  case MWMBottomMenuStateInactive:
    self.backgroundColor = [UIColor menuBackground];
    self.p2pButton.alpha = self.searchButton.alpha = self.bookmarksButton.alpha = 1.0;
    self.downloadBadge.alpha = 1.0;
    self.goButton.alpha = 0.0;
    self.streetLabel.alpha = 0.0;
    break;
  case MWMBottomMenuStateActive:
    self.backgroundColor = [UIColor whiteColor];
    self.p2pButton.alpha = self.searchButton.alpha = self.bookmarksButton.alpha = 1.0;
    self.downloadBadge.alpha = 0.0;
    self.goButton.alpha = 0.0;
    self.streetLabel.alpha = 0.0;
    break;
  case MWMBottomMenuStateCompact:
    if (!IPAD)
      self.p2pButton.alpha = self.searchButton.alpha = self.bookmarksButton.alpha = 0.0;
    self.downloadBadge.alpha = 0.0;
    self.goButton.alpha = 0.0;
    self.streetLabel.alpha = 0.0;
    break;
  case MWMBottomMenuStateGo:
    self.p2pButton.alpha = self.searchButton.alpha = self.bookmarksButton.alpha = 0.0;
    self.goButton.alpha = 1.0;
    self.streetLabel.alpha = 0.0;
    break;
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateText:
    self.p2pButton.alpha = self.searchButton.alpha = self.bookmarksButton.alpha = 0.0;
    self.goButton.alpha = 0.0;
    self.streetLabel.alpha = 1.0;
    break;
  }
}

- (void)updateVisibility
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden:
    break;
  case MWMBottomMenuStateInactive:
    self.separator.hidden = self.additionalButtons.hidden = YES;
    self.goButton.hidden = YES;
    self.streetLabel.hidden = YES;
    break;
  case MWMBottomMenuStateActive:
    self.downloadBadge.hidden = YES;
    self.goButton.hidden = YES;
    self.streetLabel.hidden = YES;
    break;
  case MWMBottomMenuStateCompact:
    if (!IPAD)
      self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = YES;
    self.downloadBadge.hidden = YES;
    self.goButton.hidden = YES;
    self.streetLabel.hidden = YES;
    break;
  case MWMBottomMenuStatePlanning:
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = YES;
    self.streetLabel.hidden = YES;
    break;
  case MWMBottomMenuStateGo:
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = YES;
    [self.goButton setBackgroundColor:[UIColor linkBlue] forState:UIControlStateNormal];
    [self.goButton setBackgroundColor:[UIColor linkBlueDark] forState:UIControlStateHighlighted];
    self.streetLabel.hidden = YES;
    break;
  case MWMBottomMenuStateText:
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = YES;
    self.goButton.hidden = YES;
    break;
  }
}

- (void)layoutWidgets
{
  UIView * superView = self.superview;
  CGFloat const contentScaleFactor = superView.contentScaleFactor;
  m2::PointD const pivot(superView.width * contentScaleFactor - 36.0,
                         (superView.height - self.mainButtons.height) * contentScaleFactor - 24.0);
  auto & infoDisplay = GetFramework().GetInformationDisplay();
  infoDisplay.SetWidgetPivot(InformationDisplay::WidgetType::Ruler, pivot);
  infoDisplay.SetWidgetPivot(InformationDisplay::WidgetType::CopyrightLabel, pivot);
}

- (void)layoutGeometry
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden:
    self.minY = self.superview.height;
    return;
  case MWMBottomMenuStateInactive:
  case MWMBottomMenuStateCompact:
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateGo:
  case MWMBottomMenuStateText:
    self.separator.height = self.additionalButtons.height = 0.0;
    break;
  case MWMBottomMenuStateActive:
    self.additionalButtons.height = self.width > self.layoutThreshold ? 64.0 : 148.0;
    break;
  }
  CGFloat const width = MIN(self.superview.width - self.leftBound, self.superview.width);
  CGFloat const height = self.mainButtons.height + self.separator.height + self.additionalButtons.height;
  self.frame = {{self.superview.width - width, self.superview.height - height}, {width, height}};
  self.mainButtonWidth.constant = self.separatorWidth.constant = self.additionalButtonsWidth.constant = width;
}

- (void)updateMenuButtonFromState:(MWMBottomMenuState)fromState toState:(MWMBottomMenuState)toState
{
  if (fromState == MWMBottomMenuStateActive || toState == MWMBottomMenuStateActive)
    [self morphMenuButtonTemplate:@"ic_menu_" toState:toState];
  else if (fromState == MWMBottomMenuStateCompact || toState == MWMBottomMenuStateCompact)
    [self morphMenuButtonTemplate:@"ic_menu_rotate_" toState:toState];
  [self refreshMenuButtonState];
}

- (void)morphMenuButtonTemplate:(NSString *)morphTemplate toState:(MWMBottomMenuState)toState
{
  BOOL const direct = toState == MWMBottomMenuStateActive || toState == MWMBottomMenuStateCompact;
  UIButton * btn = self.menuButton;
  NSUInteger const morphImagesCount = 6;
  NSUInteger const startValue = direct ? 1 : morphImagesCount;
  NSUInteger const endValue = direct ? morphImagesCount + 1 : 0;
  NSInteger const stepValue = direct ? 1 : -1;
  NSMutableArray * morphImages = [NSMutableArray arrayWithCapacity:morphImagesCount];
  for (NSUInteger i = startValue, j = 0; i != endValue; i += stepValue, j++)
    morphImages[j] = [UIImage imageNamed:[morphTemplate stringByAppendingString:@(i).stringValue]];
  btn.imageView.animationImages = morphImages;
  btn.imageView.animationRepeatCount = 1;
  btn.imageView.image = morphImages.lastObject;
  [btn.imageView startAnimating];
}

- (void)refreshMenuButtonState
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    if (self.menuButton.imageView.isAnimating)
    {
      [self refreshMenuButtonState];
    }
    else
    {
      UIButton * btn = self.menuButton;
      switch (self.state)
      {
      case MWMBottomMenuStateHidden:
      case MWMBottomMenuStateInactive:
      case MWMBottomMenuStatePlanning:
      case MWMBottomMenuStateGo:
      case MWMBottomMenuStateText:
        [btn setImage:[UIImage imageNamed:@"ic_menu"] forState:UIControlStateNormal];
        [btn setImage:[UIImage imageNamed:@"ic_menu_press"] forState:UIControlStateHighlighted];
        break;
      case MWMBottomMenuStateActive:
        [btn setImage:[UIImage imageNamed:@"ic_menu_down"] forState:UIControlStateNormal];
        [btn setImage:[UIImage imageNamed:@"ic_menu_down_press"]
             forState:UIControlStateHighlighted];
        break;
      case MWMBottomMenuStateCompact:
        [btn setImage:[UIImage imageNamed:@"ic_menu_left"] forState:UIControlStateNormal];
        [btn setImage:[UIImage imageNamed:@"ic_menu_left_press"]
             forState:UIControlStateHighlighted];
        break;
      }
    }
  });
}

- (void)refreshOnOrientationChange
{
  if (IPAD || self.state != MWMBottomMenuStateCompact)
    return;
  BOOL const isPortrait = self.superview.width < self.superview.height;
  if (isPortrait)
    self.leftBound = 0.0;
}

#pragma mark - Properties

- (void)setFrame:(CGRect)frame
{
  CGFloat const minWidth = self.locationButton.width + self.menuButton.width;
  frame.size.width = MAX(minWidth, frame.size.width);
  super.frame = frame;
}

- (void)setState:(MWMBottomMenuState)state
{
  if (_state == state)
    return;
  self.layoutDuration = kDefaultAnimationDuration;
  switch (state)
  {
  case MWMBottomMenuStateHidden:
    break;
  case MWMBottomMenuStateInactive:
    if (MapsAppDelegate.theApp.routingPlaneMode == MWMRoutingPlaneModeNone)
      _leftBound = 0.0;
    self.downloadBadge.hidden =
        GetFramework().GetCountryTree().GetActiveMapLayout().GetOutOfDateCount() == 0;
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = NO;
    self.layoutDuration =
        (_state == MWMBottomMenuStateCompact && !IPAD) ? 0.0 : kDefaultAnimationDuration;
    if (_state != MWMBottomMenuStateGo && _state != MWMBottomMenuStatePlanning &&
        _state != MWMBottomMenuStateText)
      [self updateMenuButtonFromState:_state toState:state];
    break;
  case MWMBottomMenuStateActive:
    self.restoreState = _state;
    [self updateMenuButtonFromState:_state toState:state];
    self.separator.hidden = self.additionalButtons.hidden = NO;
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = NO;
    break;
  case MWMBottomMenuStateCompact:
    self.layoutDuration = IPAD ? kDefaultAnimationDuration : 0.0;
    [self updateMenuButtonFromState:_state toState:state];
    break;
  case MWMBottomMenuStatePlanning:
    self.streetLabel.font = [UIFont regular17];
    self.streetLabel.textColor = [UIColor blackHintText];
    self.streetLabel.text = L(@"routing_planning");
    self.streetLabel.hidden = NO;
    [self updateMenuButtonFromState:_state toState:state];
    break;
  case MWMBottomMenuStateGo:
    self.goButton.hidden = NO;
    [self updateMenuButtonFromState:_state toState:state];
    break;
  case MWMBottomMenuStateText:
    self.streetLabel.font = [UIFont medium16];
    self.streetLabel.textColor = [UIColor blackSecondaryText];
    self.streetLabel.hidden = NO;
    [self updateMenuButtonFromState:_state toState:state];
    break;
  }
  _state = state;
  [self setNeedsLayout];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = MAX(leftBound, 0.0);
  self.state = _leftBound > 1.0 ? MWMBottomMenuStateCompact : self.restoreState;
  [self setNeedsLayout];
}

- (void)setSearchIsActive:(BOOL)searchIsActive
{
  _searchIsActive = self.searchButton.selected = searchIsActive;
}

@end
