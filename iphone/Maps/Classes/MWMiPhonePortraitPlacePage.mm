#import "Common.h"
#import "MapViewController.h"
#import "MWMBasePlacePageView.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePage+Animation.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMSpringAnimation.h"

#include "Framework.h"

static CGFloat const kPlacePageBottomOffset = 31.;

typedef NS_ENUM(NSUInteger, MWMiPhonePortraitPlacePageState)
{
  MWMiPhonePortraitPlacePageStateClosed,
  MWMiPhonePortraitPlacePageStatePreview,
  MWMiPhonePortraitPlacePageStateOpen,
  MWMiPhonePortraitPlacePageStateHover
};

@interface MWMiPhonePortraitPlacePage ()

@property (nonatomic) MWMiPhonePortraitPlacePageState state;
@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) CGFloat panVelocity;

@end

@implementation MWMiPhonePortraitPlacePage

- (void)configure
{
  [super configure];
  self.basePlacePageView.featureTable.scrollEnabled = NO;
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const width = MIN(size.width, size.height);
  CGFloat const height = MAX(size.width, size.height);
  UIView * ppv = self.extendedPlacePageView;
  ppv.frame = CGRectMake(0., height, width, 2 * height);
  _targetPoint = ppv.center;
  self.actionBar.width = width;
  self.actionBar.center = CGPointMake(width / 2., height + self.actionBar.height / 2.);
  [self.manager addSubviews:@[ppv, self.actionBar] withNavigationController:nil];
  [UIView animateWithDuration:kDefaultAnimationDuration delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
  {
    self.actionBar.center = CGPointMake(width / 2., height - self.actionBar.height / 2.);
  }
  completion:nil];
}

- (void)show
{
  self.state = MWMiPhonePortraitPlacePageStatePreview;
}

- (void)hide
{
  self.state = MWMiPhonePortraitPlacePageStateClosed;
}

- (void)dismiss
{
  [MWMPlacePageNavigationBar remove];
  [super dismiss];
}

- (void)addBookmark
{
  [super addBookmark];
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)removeBookmark
{
 [super removeBookmark];
  self.state = MWMiPhonePortraitPlacePageStatePreview;
}

- (void)reloadBookmark
{
  [super reloadBookmark];
  [self updateTargetPoint];
}

- (void)updateMyPositionStatus:(NSString *)status
{
  [super updateMyPositionStatus:status];
  [self updateTargetPoint];
}

- (void)setState:(MWMiPhonePortraitPlacePageState)state
{
  _state = state;
  [self updateTargetPoint];
  switch (state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
      [self.actionBar removeFromSuperview];
      [MWMPlacePageNavigationBar remove];
      [self.manager.ownerViewController.view endEditing:YES];
      break;
    case MWMiPhonePortraitPlacePageStatePreview:
      [MWMPlacePageNavigationBar remove];
      [self.manager.ownerViewController.view endEditing:YES];
      break;
    case MWMiPhonePortraitPlacePageStateOpen:
    case MWMiPhonePortraitPlacePageStateHover:
      break;
  }
  [self setAnchorImage];
}

- (void)updateTargetPoint
{
  UIView * ppv = self.extendedPlacePageView;
  switch (self.state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
      self.targetPoint = CGPointMake(ppv.width / 2., ppv.height * 2.);
      break;
    case MWMiPhonePortraitPlacePageStatePreview:
      self.targetPoint = [self getPreviewTargetPoint];
      break;
    case MWMiPhonePortraitPlacePageStateOpen:
      self.targetPoint = [self getOpenTargetPoint];
      break;
    case MWMiPhonePortraitPlacePageStateHover:
      self.targetPoint = CGPointMake(ppv.center.x, MAX([MWMSpringAnimation approxTargetFor:ppv.center.y velocity:self.panVelocity], [self getOpenTargetPoint].y));
      break;
  }
}

- (CGPoint)getPreviewTargetPoint
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  BOOL const isLandscape = size.width > size.height;
  CGFloat const width = isLandscape ? size.height : size.width;
  CGFloat const height = isLandscape ? size.width : size.height;
  MWMBasePlacePageView * basePPV = self.basePlacePageView;
  CGFloat const typeHeight = basePPV.typeLabel.text.length > 0 ? basePPV.typeLabel.height : basePPV.typeDescriptionView.height;
  CGFloat const h = height - (basePPV.titleLabel.height + kPlacePageBottomOffset + typeHeight + self.actionBar.height);
  return CGPointMake(width / 2., height + h);
}

- (CGPoint)getOpenTargetPoint
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  BOOL const isLandscape = size.width > size.height;
  CGFloat const width = isLandscape ? size.height : size.width;
  CGFloat const height = isLandscape ? size.width : size.height;
  MWMBasePlacePageView * basePPV = self.basePlacePageView;
  CGFloat const typeHeight = basePPV.typeLabel.text.length > 0 ? basePPV.typeLabel.height : basePPV.typeDescriptionView.height;
  CGFloat const h = height - (basePPV.titleLabel.height + kPlacePageBottomOffset + typeHeight + [(UITableView *)basePPV.featureTable height] + self.actionBar.height + self.keyboardHeight);
  return CGPointMake(width / 2., height + h);
}

#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  UIView * ppv = self.extendedPlacePageView;
  UIView * ppvSuper = ppv.superview;

  ppv.midY = MAX(ppv.midY + [sender translationInView:ppvSuper].y, [self getOpenTargetPoint].y);
  _targetPoint = ppv.center;
  if (ppv.minY <= 0.0)
    [MWMPlacePageNavigationBar showNavigationBarForPlacePage:self];
  else
    [MWMPlacePageNavigationBar dismissNavigationBar];
  [sender setTranslation:CGPointZero inView:ppvSuper];
  [self cancelSpringAnimation];
  UIGestureRecognizerState const state = sender.state;
  if (state == UIGestureRecognizerStateEnded || state == UIGestureRecognizerStateCancelled)
  {
    self.panVelocity = [sender velocityInView:ppvSuper].y;
    CGFloat const estimatedYPosition = [MWMSpringAnimation approxTargetFor:ppv.frame.origin.y velocity:self.panVelocity];
    CGFloat const bound1 = ppvSuper.height * 0.2;
    CGFloat const bound2 = ppvSuper.height * 0.5;
    if (estimatedYPosition < bound1)
    {
      if (self.panVelocity <= 0.0)
        self.state = MWMiPhonePortraitPlacePageStateHover;
      else
        self.state = MWMiPhonePortraitPlacePageStateOpen;
    }
    else if (self.panVelocity <= 0.0)
    {
      self.state = MWMiPhonePortraitPlacePageStateOpen;
    }
    else if (ppv.minY < bound2)
    {
      self.state = MWMiPhonePortraitPlacePageStatePreview;
    }
    else
    {
      [self.manager dismissPlacePage];
    }
  }
}

- (IBAction)didTap:(UITapGestureRecognizer *)sender
{
  [super didTap:sender];
  switch (self.state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
      self.state = MWMiPhonePortraitPlacePageStatePreview;
      break;
    case MWMiPhonePortraitPlacePageStatePreview:
      self.state = MWMiPhonePortraitPlacePageStateOpen;
      break;
    case MWMiPhonePortraitPlacePageStateOpen:
    case MWMiPhonePortraitPlacePageStateHover:
      self.state = MWMiPhonePortraitPlacePageStatePreview;
      break;
  }
}

- (void)willStartEditingBookmarkTitle
{
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)willFinishEditingBookmarkTitle:(NSString *)title
{
  [super willFinishEditingBookmarkTitle:title];
  [self updateTargetPoint];
}

- (void)setAnchorImage
{
  NSString * anchorImageName = nil;
  switch (self.state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
      break;
    case MWMiPhonePortraitPlacePageStatePreview:
      anchorImageName = @"bg_placepage_tablet_normal_";
      break;
    case MWMiPhonePortraitPlacePageStateOpen:
    case MWMiPhonePortraitPlacePageStateHover:
      anchorImageName = @"bg_placepage_tablet_open_";
      break;
  }
  if (anchorImageName)
  {
    CGSize const size = UIScreen.mainScreen.bounds.size;
    CGFloat const width = MIN(size.height, size.width);
    self.anchorImageView.image = [UIImage imageNamed:[anchorImageName stringByAppendingString:@((NSUInteger)width).stringValue]];
  }
}

#pragma mark - Properties

- (void)setTargetPoint:(CGPoint)targetPoint
{
  if (CGPointEqualToPoint(_targetPoint, targetPoint))
    return;
  _targetPoint = targetPoint;
  __weak MWMiPhonePortraitPlacePage * weakSelf = self;
  if (self.state == MWMiPhonePortraitPlacePageStateClosed)
    GetFramework().GetBalloonManager().RemovePin();
  [self startAnimatingPlacePage:self initialVelocity:CGPointMake(0.0, self.panVelocity) completion:^
  {
    __strong MWMiPhonePortraitPlacePage * self = weakSelf;
    if (self.state == MWMiPhonePortraitPlacePageStateClosed)
    {
      [self.manager dismissPlacePage];
    }
    else
    {
      if (self.extendedPlacePageView.minY <= 0.0)
        [MWMPlacePageNavigationBar showNavigationBarForPlacePage:self];
      else
        [MWMPlacePageNavigationBar dismissNavigationBar];
    }
  }];
  self.panVelocity = 0.0;
}

@end
