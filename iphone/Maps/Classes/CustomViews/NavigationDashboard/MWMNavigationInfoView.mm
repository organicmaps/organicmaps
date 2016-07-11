#import "MWMNavigationInfoView.h"
#import "Common.h"
#import "UIFont+MapsMeFonts.h"
#import "UIImageView+Coloring.h"

namespace
{
CGFloat constexpr kTurnsiPhoneWidth = 96;
CGFloat constexpr kTurnsiPadWidth = 140;
}  // namespace

@interface MWMNavigationInfoView ()

@property(weak, nonatomic) IBOutlet UIView * streetNameView;
@property(weak, nonatomic) IBOutlet UILabel * streetNameLabel;
@property(weak, nonatomic) IBOutlet UIView * turnsView;
@property(weak, nonatomic) IBOutlet UIImageView * nextTurnImageView;
@property(weak, nonatomic) IBOutlet UILabel * distanceToNextTurnLabel;
@property(weak, nonatomic) IBOutlet UIView * secondTurnView;
@property(weak, nonatomic) IBOutlet UIImageView * secondTurnImageView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * turnsWidth;

@property(nonatomic) BOOL isVisible;

@end

@implementation MWMNavigationInfoView

- (void)addToView:(UIView *)superview
{
  self.isVisible = YES;
  if (IPAD)
  {
    self.turnsWidth.constant = kTurnsiPadWidth;
    self.distanceToNextTurnLabel.font = [UIFont bold36];
  }
  else
  {
    self.turnsWidth.constant = kTurnsiPhoneWidth;
    self.distanceToNextTurnLabel.font = [UIFont bold24];
  }
  NSAssert(superview != nil, @"Superview can't be nil");
  if ([superview.subviews containsObject:self])
    return;
  [superview insertSubview:self atIndex:0];
}

- (void)remove { self.isVisible = NO; }
- (void)layoutSubviews
{
  if (!CGRectEqualToRect(self.frame, self.superview.bounds))
  {
    self.frame = self.superview.bounds;
    [self setNeedsLayout];
  }
  if (!self.isVisible)
    [self removeFromSuperview];
  [super layoutSubviews];
}

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = isVisible;
  [self setNeedsLayout];
}

- (CGFloat)visibleHeight { return self.streetNameView.maxY; }
#pragma mark - MWMNavigationDashboardInfoProtocol

- (void)updateNavigationInfo:(MWMNavigationDashboardEntity *)info
{
  if (info.streetName.length != 0)
  {
    self.streetNameView.hidden = NO;
    self.streetNameLabel.text = info.streetName;
  }
  else
  {
    self.streetNameView.hidden = YES;
  }
  if (info.turnImage)
  {
    self.turnsView.hidden = NO;
    self.nextTurnImageView.image = info.turnImage;
    if (isIOS7)
      [self.nextTurnImageView makeImageAlwaysTemplate];
    self.nextTurnImageView.mwm_coloring = MWMImageColoringWhite;
    self.distanceToNextTurnLabel.text =
        [NSString stringWithFormat:@"%@%@", info.distanceToTurn, info.turnUnits];
    if (info.nextTurnImage)
    {
      self.secondTurnView.hidden = NO;
      self.secondTurnImageView.image = info.nextTurnImage;
      if (isIOS7)
        [self.secondTurnImageView makeImageAlwaysTemplate];
      self.secondTurnImageView.mwm_coloring = MWMImageColoringBlack;
    }
    else
    {
      self.secondTurnView.hidden = YES;
    }
  }
  else
  {
    self.turnsView.hidden = YES;
  }
  self.hidden = self.streetNameView.hidden && self.turnsView.hidden;
}

@end
