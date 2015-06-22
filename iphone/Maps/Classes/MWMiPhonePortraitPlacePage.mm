//
//  MWMPlacePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 17.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMiPhonePortraitPlacePage.h"
#import "MWMSpringAnimation.h"
#import "UIKitCategories.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePage+Animation.h"
#import "MWMPlacePageEntity.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"

#include "Framework.h"

static NSString * const kPlacePageViewDragKeyPath = @"center";
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
@property (nonatomic) CGFloat keyboardHeight;
@property (nonatomic) CGFloat panVelocity;

@end

@implementation MWMiPhonePortraitPlacePage

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager
{
  self = [super initWithManager:manager];
  if (self)
  {
    [self.extendedPlacePageView addObserver:self forKeyPath:kPlacePageViewDragKeyPath options:NSKeyValueObservingOptionNew context:nullptr];
  }
  return self;
}

- (void)dealloc
{
  [self.extendedPlacePageView removeObserver:self forKeyPath:kPlacePageViewDragKeyPath];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if ([self.extendedPlacePageView isEqual:object] && [keyPath isEqualToString:kPlacePageViewDragKeyPath])
    [self.manager dragPlacePage:self.extendedPlacePageView.origin];
}

- (void)configure
{
  [super configure];
  self.basePlacePageView.featureTable.scrollEnabled = NO;
  UIView const * view = self.manager.ownerViewController.view;
  if ([view.subviews containsObject:self.extendedPlacePageView])
    return;

  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const width = MIN(size.width, size.height);
  CGFloat const height = MAX(size.width, size.height);
  self.extendedPlacePageView.frame = CGRectMake(0., height, width, 2 * height);
  [view addSubview:self.extendedPlacePageView];
  self.actionBar.width = width;
  self.actionBar.center = CGPointMake(width / 2., height + self.actionBar.height / 2.);
  [view addSubview:self.actionBar];

  [UIView animateWithDuration:0.3f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
  {
    self.actionBar.center = CGPointMake(width / 2., height - self.actionBar.height / 2.);
  }
  completion:nil];
}

- (void)show
{
  self.state = MWMiPhonePortraitPlacePageStatePreview;
}

- (void)dismiss
{
  self.state = MWMiPhonePortraitPlacePageStateClosed;
}

- (void)addBookmark
{
  [super addBookmark];
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)removeBookmark
{
 [super removeBookmark];
  self.keyboardHeight = 0.;
  self.state = MWMiPhonePortraitPlacePageStatePreview;
}

- (void)updateMyPositionStatus:(NSString *)status
{
  [super updateMyPositionStatus:status];
  [self updateTargetPoint];
}

- (void)setState:(MWMiPhonePortraitPlacePageState)state
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const width = MIN(size.height, size.width);
  NSString * anchorImageName;
  _state = state;
  [self updateTargetPoint];
  switch (state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
      [self.actionBar removeFromSuperview];
      break;
    case MWMiPhonePortraitPlacePageStatePreview:
      anchorImageName = @"bg_placepage_tablet_normal_";
      break;
    case MWMiPhonePortraitPlacePageStateOpen:
    case MWMiPhonePortraitPlacePageStateHover:
      anchorImageName = @"bg_placepage_tablet_open_";
      break;
  }
  self.anchorImageView.image = [UIImage imageNamed:[anchorImageName stringByAppendingString:@((NSUInteger)width).stringValue]];
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

  ppv.minY += [sender translationInView:ppvSuper].y;
  ppv.midY = MAX(ppv.midY, [self getOpenTargetPoint].y);
  if (ppv.minY <= 0.0)
    [MWMPlacePageNavigationBar showNavigationBarForPlacePage:self];
  else
    [MWMPlacePageNavigationBar dismissNavigationBar];

  [sender setTranslation:CGPointZero inView:ppvSuper];
  if (sender.state == UIGestureRecognizerStateEnded)
  {
    self.panVelocity = [sender velocityInView:ppvSuper].y;
    CGFloat const estimatedYPosition = [MWMSpringAnimation approxTargetFor:ppv.frame.origin.y velocity:self.panVelocity];
    CGFloat const bound1 = ppvSuper.height * 0.2;
    CGFloat const bound2 = ppvSuper.height * 0.5;
    if (estimatedYPosition < bound1)
      self.state = MWMiPhonePortraitPlacePageStateHover;
    else if (self.panVelocity <= 0.0)
      self.state = MWMiPhonePortraitPlacePageStateOpen;
    else if (ppv.minY < bound2)
      self.state = MWMiPhonePortraitPlacePageStatePreview;
    else
      [self.manager dismissPlacePage];
  }
  else
  {
    [self cancelSpringAnimation];
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
      [self.manager.ownerViewController.view endEditing:YES];
      break;
  }
}

- (void)willStartEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  self.keyboardHeight = keyboardHeight;
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)willFinishEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  self.keyboardHeight = 0.;
  [self updateTargetPoint];
}

#pragma mark - Properties

- (void)setTargetPoint:(CGPoint)targetPoint
{
  if (CGPointEqualToPoint(_targetPoint, targetPoint))
    return;
  _targetPoint = targetPoint;
  __weak MWMiPhonePortraitPlacePage * weakSelf = self;
  [self startAnimatingPlacePage:self initialVelocity:CGPointMake(0.0, self.panVelocity) completion:^
  {
    __strong MWMiPhonePortraitPlacePage * self = weakSelf;
    if (self.state == MWMiPhonePortraitPlacePageStateClosed)
    {
      [MWMPlacePageNavigationBar remove];
      self.keyboardHeight = 0.;
      [super dismiss];
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
