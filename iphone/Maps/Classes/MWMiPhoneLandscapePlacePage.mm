#import "Common.h"
#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMSpringAnimation.h"
#import "MWMPlacePage+Animation.h"

#include "Framework.h"

static CGFloat const kMaximumPlacePageWidth = 360.;
extern CGFloat const kBookmarkCellHeight;

typedef NS_ENUM(NSUInteger, MWMiPhoneLandscapePlacePageState)
{
  MWMiPhoneLandscapePlacePageStateClosed,
  MWMiPhoneLandscapePlacePageStateOpen
};

@interface MWMiPhoneLandscapePlacePage ()

@property (nonatomic) MWMiPhoneLandscapePlacePageState state;
@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) CGFloat panVelocity;

@end

@implementation MWMiPhoneLandscapePlacePage

- (void)configure
{
  [super configure];
  self.anchorImageView.backgroundColor = [UIColor whiteColor];
  self.anchorImageView.image = nil;
  [self configureContentInset];
  [self addPlacePageShadowToView:self.extendedPlacePageView offset:CGSizeMake(2.0, 4.0)];
  [self.extendedPlacePageView addSubview:self.actionBar];
  [self.manager addSubviews:@[self.extendedPlacePageView] withNavigationController:nil];
}

- (void)show
{
  if (self.state == MWMiPhoneLandscapePlacePageStateOpen)
    return;
  CGSize const size = self.extendedPlacePageView.superview.size;
  CGFloat const height = MIN(size.width, size.height);
  CGFloat const offset = MIN(height, kMaximumPlacePageWidth);
  self.extendedPlacePageView.minX = -offset;
  self.actionBar.width = offset;
  self.actionBar.minX = 0.0;
  self.state = MWMiPhoneLandscapePlacePageStateOpen;
}

- (void)hide
{
  if (self.state == MWMiPhoneLandscapePlacePageStateClosed)
    return;
  self.state = MWMiPhoneLandscapePlacePageStateClosed;
}

- (void)configureContentInset
{
  CGFloat const height = self.extendedPlacePageView.height - self.anchorImageView.height;
  CGFloat const actionBarHeight = self.actionBar.height;
  UITableView * featureTable = self.basePlacePageView.featureTable;
  CGFloat const tableContentHeight = featureTable.contentSize.height;
  CGFloat const headerViewHeight = self.basePlacePageView.separatorView.maxY;
  CGFloat const availableTableHeight = height - headerViewHeight - actionBarHeight;
  CGFloat const externalHeight = tableContentHeight - availableTableHeight;
  if (externalHeight > 0)
  {
    featureTable.contentInset = UIEdgeInsetsMake(0., 0., externalHeight, 0.);
    featureTable.scrollEnabled = YES;
  }
  else
  {
    [featureTable setContentOffset:CGPointZero animated:YES];
    featureTable.scrollEnabled = NO;
  }
}

- (void)addBookmark
{
  [super addBookmark];
  [self configureContentInset];
}

- (void)removeBookmark
{
  [super removeBookmark];
  [self configureContentInset];
}

- (void)updateTargetPoint
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = MIN(size.width, size.height);
  CGFloat const offset = MIN(height, kMaximumPlacePageWidth);
  switch (self.state)
  {
    case MWMiPhoneLandscapePlacePageStateClosed:
      self.targetPoint = CGPointMake(-offset / 2., (height + self.topBound) / 2.);
      break;
    case MWMiPhoneLandscapePlacePageStateOpen:
      self.targetPoint = CGPointMake(offset / 2., (height + self.topBound) / 2.);
      break;
  }
}
#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  UIView * ppv = self.extendedPlacePageView;
  UIView * ppvSuper = ppv.superview;
  ppv.minX += [sender translationInView:ppvSuper].x;
  ppv.minX = MIN(ppv.minX, 0.0);
  [sender setTranslation:CGPointZero inView:ppvSuper];
  [self cancelSpringAnimation];
  UIGestureRecognizerState const state = sender.state;
  if (state == UIGestureRecognizerStateEnded || state == UIGestureRecognizerStateCancelled)
  {
    sender.enabled = NO;
    self.panVelocity = [sender velocityInView:ppvSuper].x;
    if (self.panVelocity > 0)
      [self show];
    else
      [self hide];
  }
}

- (void)willStartEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  [super willStartEditingBookmarkTitle:keyboardHeight];
  CGFloat const statusBarHeight = [[UIApplication sharedApplication] statusBarFrame].size.height;
  MWMBasePlacePageView const * basePlacePageView = self.basePlacePageView;
  UITableView const * tableView = basePlacePageView.featureTable;
  CGFloat const baseViewHeight = basePlacePageView.height;
  CGFloat const tableHeight = tableView.contentSize.height;
  CGFloat const headerViewHeight = baseViewHeight - tableHeight;
  CGFloat const titleOriginY = tableHeight - kBookmarkCellHeight - tableView.contentOffset.y;

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.basePlacePageView.transform = CGAffineTransformMakeTranslation(0., statusBarHeight - headerViewHeight - titleOriginY);
  }];
}

- (void)willFinishEditingBookmarkTitle:(NSString *)title
{
  [super willFinishEditingBookmarkTitle:title];
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.basePlacePageView.transform = CGAffineTransformMakeTranslation(0., 0.);
  }];
}

#pragma mark - Properties

- (void)setState:(MWMiPhoneLandscapePlacePageState)state
{
  if (_state == state)
    return;
  _state = state;
  [self updateTargetPoint];
}

- (void)setTopBound:(CGFloat)topBound
{
  super.topBound = topBound;
  CGSize const size = self.extendedPlacePageView.superview.size;
  CGFloat const height = MIN(size.width, size.height);
  CGRect const frame = self.extendedPlacePageView.frame;
  self.extendedPlacePageView.frame = {{frame.origin.x, topBound}, {frame.size.width, height - topBound}};
  self.actionBar.maxY = height - topBound;
  if (self.state == MWMiPhoneLandscapePlacePageStateOpen)
    [self updateTargetPoint];
  [self configureContentInset];
}

- (void)setTargetPoint:(CGPoint)targetPoint
{
  _targetPoint = targetPoint;
  __weak MWMiPhoneLandscapePlacePage * weakSelf = self;
  if (self.state == MWMiPhoneLandscapePlacePageStateClosed)
    GetFramework().GetBalloonManager().RemovePin();
  [self startAnimatingPlacePage:self initialVelocity:CGPointMake(self.panVelocity, 0.0) completion:^
  {
    __strong MWMiPhoneLandscapePlacePage * self = weakSelf;
    if (self.state == MWMiPhoneLandscapePlacePageStateClosed)
      [self.manager dismissPlacePage];
    else
      self.panRecognizer.enabled = YES;
  }];
  self.panVelocity = 0.0;
}

@end
