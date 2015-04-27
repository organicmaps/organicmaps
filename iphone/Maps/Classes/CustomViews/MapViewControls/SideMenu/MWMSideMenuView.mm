//
//  MWMSideMenuView.m
//  Maps
//
//  Created by Ilya Grechuhin on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMSideMenuView.h"
#import "UIKitCategories.h"
#import "MWMMapViewControlsCommon.h"
#import "Framework.h"

static CGSize const kBadgeSize = CGSizeMake(24.0, 24.0);

@interface MWMSideMenuView()

@property (weak, nonatomic, readwrite) IBOutlet UIView * dimBackground;

@property (weak, nonatomic) IBOutlet UIButton * bookmarksButton;
@property (weak, nonatomic) IBOutlet UIButton * downloadMapsButton;
@property (weak, nonatomic) IBOutlet UIButton * settingsButton;
@property (weak, nonatomic) IBOutlet UIButton * shareLocationButton;
@property (weak, nonatomic) IBOutlet UIButton * searchButton;

@property (weak, nonatomic) IBOutlet UIButton * bookmarksLabel;
@property (weak, nonatomic) IBOutlet UIButton * downloadMapsLabel;
@property (weak, nonatomic) IBOutlet UIButton * settingsLabel;
@property (weak, nonatomic) IBOutlet UIButton * shareLocationLabel;
@property (weak, nonatomic) IBOutlet UIButton * searchLabel;

@property (weak, nonatomic) IBOutlet UIImageView * downloadBadge;
@property (weak, nonatomic) IBOutlet UILabel * downloadCount;

@end

@implementation MWMSideMenuView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  return self;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  // Prevent super call to stop event propagation
  // [super touchesBegan:touches withEvent:event];
}

- (void)setup
{
  UIImageView const * const animationIV = self.searchButton.imageView;
  NSString const * const imageName = @"btn_green_menu_";
  static NSUInteger const animationImagesCount = 4;
  NSMutableArray * const animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"%@%@", imageName, @(i+1)]];
  animationIV.animationImages = animationImages;
  animationIV.animationDuration = framesDuration(animationIV.animationImages.count);
  animationIV.animationRepeatCount = 1;
  [animationIV startAnimating];
  [self setNeedsLayout];
}

- (void)addSelfToView:(UIView *)parentView
{
  if (self.superview == parentView)
    return;
  [self setup];
  [parentView addSubview:self];
  self.frame = parentView.bounds;
  self.alpha = 1.0;
  self.downloadBadge.hidden = YES;
  self.downloadCount.hidden = YES;
  [self updateMenuBackground];
  [self updateMenuUI];
}

- (void)removeFromSuperviewAnimated
{
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    [super removeFromSuperview];
  }];
}

- (void)layoutSubviews
{
  self.frame = self.superview.frame;
  self.dimBackground.frame = self.frame;
  
  CGSize const boundsSize = self.size;
  NSUInteger const buttonsCount = 5;
  CGFloat offsetBetweenButtons = 66.0;
  CGFloat const offsetLabelToButton = 10.0;
  CGFloat const buttonCenterOffsetToBounds = 0.5 * self.searchButton.height + 2.0 * kViewControlsOffsetToBounds;
  
  if (boundsSize.width > boundsSize.height)
  {
    CGFloat const buttonHeight = boundsSize.height;
    offsetBetweenButtons = MIN(offsetBetweenButtons, (boundsSize.height - 2.0 * (buttonCenterOffsetToBounds - 0.5 * buttonHeight) - buttonsCount*buttonHeight) / (buttonsCount - 1) + buttonHeight);
  }
  
  CGPoint buttonCenter = CGPointMake(boundsSize.width - buttonCenterOffsetToBounds, boundsSize.height - buttonCenterOffsetToBounds);
  
  self.searchButton.center = buttonCenter;
  self.searchLabel.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.shareLocationButton.center = buttonCenter;
  self.shareLocationLabel.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.settingsButton.center = buttonCenter;
  self.settingsLabel.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.downloadMapsButton.center = buttonCenter;
  self.downloadMapsLabel.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.bookmarksButton.center = buttonCenter;
  self.bookmarksLabel.center = buttonCenter;

  CGFloat const labelWidth = 0.5 * boundsSize.width;
  self.shareLocationLabel.width = labelWidth;
  self.settingsLabel.width = labelWidth;
  self.downloadMapsLabel.width = labelWidth;
  self.bookmarksLabel.width = labelWidth;

  CGFloat const rightBound = self.shareLocationButton.minX - offsetLabelToButton;
  self.shareLocationLabel.maxX = rightBound;
  self.settingsLabel.maxX = rightBound;
  self.downloadMapsLabel.maxX = rightBound;
  self.bookmarksLabel.maxX = rightBound;
  self.searchLabel.maxX = rightBound;
  
  self.downloadBadge.maxX = self.downloadMapsButton.maxX;
  self.downloadCount.maxX = self.downloadMapsButton.maxX;
  self.downloadBadge.minY = self.downloadMapsButton.minY;
  self.downloadCount.minY = self.downloadMapsButton.minY;
}

#pragma mark - Animations

- (void)updateMenuBackground
{
  self.dimBackground.hidden = NO;
  self.dimBackground.alpha = 0.0;
  [UIView animateWithDuration:framesDuration(2) animations:^
  {
    self.dimBackground.alpha = 0.8;
  }];
}

- (void)updateMenuUI
{
  [self showItem:self.searchLabel button:nil delay:framesDuration(0)];
  [self showItem:self.shareLocationLabel button:self.shareLocationButton delay:framesDuration(1.5)];
  [self showItem:self.settingsLabel button:self.settingsButton delay:framesDuration(3)];
  [self showItem:self.downloadMapsLabel button:self.downloadMapsButton delay:framesDuration(4.5)];
  [self showItem:self.bookmarksLabel button:self.bookmarksButton delay:framesDuration(6)];
  
  [self performSelector:@selector(updateMenuOutOfDateBadge) withObject:nil afterDelay:framesDuration(10)];
}

- (void)updateMenuOutOfDateBadge
{
  int const outOfDateCount = GetFramework().GetCountryTree().GetActiveMapLayout().GetOutOfDateCount();
  if (outOfDateCount == 0)
    return;
  CATransform3D const zeroScale = CATransform3DScale(CATransform3DIdentity, 0.0, 0.0, 1.0);
  self.downloadBadge.layer.transform = zeroScale;
  self.downloadCount.layer.transform = zeroScale;
  self.downloadBadge.alpha = 0.0;
  self.downloadCount.alpha = 0.0;
  self.downloadBadge.hidden = NO;
  self.downloadCount.hidden = NO;
  self.downloadCount.text = @(outOfDateCount).stringValue;
  [UIView animateWithDuration:framesDuration(4) animations:^
  {
    self.downloadBadge.layer.transform = CATransform3DIdentity;
    self.downloadCount.layer.transform = CATransform3DIdentity;
    self.downloadBadge.alpha = 1.0;
    self.downloadCount.alpha = 1.0;
  }];
}

- (void)showItem:(UIButton *)label button:(UIButton *)button delay:(NSTimeInterval)delay
{
  label.alpha = 0.0;
  [UIView animateWithDuration:framesDuration(5) delay:framesDuration(3) + delay options:UIViewAnimationOptionCurveLinear animations:^
  {
    label.alpha = 1.0;
  } completion:nil];
  
  if (button == nil)
    return;

  button.alpha = 0.0;
  [UIView animateWithDuration:framesDuration(6) delay:framesDuration(3) + delay options:UIViewAnimationOptionCurveLinear animations:^
  {
    button.alpha = 1.0;
  } completion:nil];

  CABasicAnimation * translationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.translation.x"];
  translationAnimation.timingFunction = [CAMediaTimingFunction functionWithControlPoints:0.55 :0.0 :0.4 :1.4];
  translationAnimation.duration = framesDuration(7);
  translationAnimation.beginTime = CACurrentMediaTime() + delay;
  translationAnimation.fromValue = @(2.0 * kViewControlsOffsetToBounds + button.size.width);
  translationAnimation.toValue = @0.0;
  [button.layer addAnimation:translationAnimation forKey:@"translationAnimation"];
}

@end
