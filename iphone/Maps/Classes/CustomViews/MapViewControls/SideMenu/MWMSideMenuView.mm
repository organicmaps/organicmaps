//
//  MWMSideMenuView.m
//  Maps
//
//  Created by Ilya Grechuhin on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMMapViewControlsCommon.h"
#import "MWMSideMenuView.h"
#import "UIKitCategories.h"

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

@property (nonatomic) NSArray * buttons;
@property (nonatomic) NSArray * labels;

@end

@implementation MWMSideMenuView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (!self)
    return nil;
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  return self;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.buttons = @[self.bookmarksButton, self.downloadMapsButton, self.settingsButton, self.shareLocationButton, self.searchButton];
  self.labels = @[self.bookmarksLabel, self.downloadMapsLabel, self.settingsLabel, self.shareLocationLabel, self.searchLabel];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  // Prevent super call to stop event propagation
  // [super touchesBegan:touches withEvent:event];
}

- (void)setup
{
  self.contentScaleFactor = self.superview.contentScaleFactor;
  self.frame = self.superview.bounds;
  self.alpha = 1.0;
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
  self.downloadBadge.hidden = YES;
  self.downloadCount.hidden = YES;
  [self updateMenuBackground];
  [self updateMenuUI];
  [self setNeedsLayout];
}

- (CGFloat)offsetBetweenButtons
{
  CGFloat offset = 66.0;
  CGFloat const buttonWidth = self.size.width;
  CGFloat const buttonHeight = self.size.height;
  if (buttonWidth > buttonHeight)
  {
    NSUInteger const buttonsCount = self.buttons.count;
    CGFloat const buttonCenterOffsetToBounds = 0.5 * self.searchButton.height + 2.0 * kViewControlsOffsetToBounds;
    offset = MIN(offset, (buttonHeight - 2.0 * (buttonCenterOffsetToBounds - 0.5 * buttonHeight) - buttonsCount*buttonHeight) / (buttonsCount - 1) + buttonHeight);
  }
  return offset;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.frame = self.superview.frame;
  self.dimBackground.frame = self.frame;
  
  CGSize const boundsSize = self.size;
  CGFloat const offsetBetweenButtons = [self offsetBetweenButtons];
  CGFloat const offsetLabelToButton = 10.0;
  CGFloat const buttonCenterOffsetToBounds = 0.5 * self.searchButton.height + 2.0 * kViewControlsOffsetToBounds;
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
  
  m2::PointD const pivot(self.searchButton.minX * self.contentScaleFactor - 2.0 * kViewControlsOffsetToBounds, self.searchButton.maxY * self.contentScaleFactor - kViewControlsOffsetToBounds);
  [self.delegate setRulerPivot:pivot];
  [self.delegate setCopyrightLabelPivot:pivot];
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
  [self showAnimation];
  [self performSelector:@selector(updateMenuOutOfDateBadge) withObject:nil afterDelay:framesDuration(10)];
}

- (void)updateMenuOutOfDateBadge
{
  if (self.outOfDateCount == 0)
    return;
  CATransform3D const zeroScale = CATransform3DScale(CATransform3DIdentity, 0.0, 0.0, 1.0);
  self.downloadBadge.layer.transform = zeroScale;
  self.downloadCount.layer.transform = zeroScale;
  self.downloadBadge.alpha = 0.0;
  self.downloadCount.alpha = 0.0;
  self.downloadBadge.hidden = NO;
  self.downloadCount.hidden = NO;
  self.downloadCount.text = @(self.outOfDateCount).stringValue;
  [UIView animateWithDuration:framesDuration(4) animations:^
  {
    self.downloadBadge.layer.transform = CATransform3DIdentity;
    self.downloadCount.layer.transform = CATransform3DIdentity;
    self.downloadBadge.alpha = 1.0;
    self.downloadCount.alpha = 1.0;
  }];
}

- (void)showAnimation
{
  [self.labels enumerateObjectsUsingBlock:^(UIButton * label, NSUInteger idx, BOOL *stop)
   {
     label.alpha = 0.0;
   }];

  [self.buttons enumerateObjectsUsingBlock:^(UIButton * button, NSUInteger idx, BOOL *stop)
  {
    button.alpha = 0.0;
  }];
  [self showAnimationStepWithLabels:[self.labels mutableCopy] buttons:[self.buttons mutableCopy]];
}

- (void)showAnimationStepWithLabels:(NSMutableArray *)labels buttons:(NSMutableArray *)buttons
{
  if (buttons.count == 0 || labels.count == 0)
    return;

  UIButton const * const label = [labels lastObject];
  [labels removeLastObject];
  label.alpha = 1.0;

  UIButton const * const button = [buttons lastObject];
  [buttons removeLastObject];
  button.alpha = 1.0;

  [CATransaction begin];
  [CATransaction setAnimationDuration:framesDuration(1.0)];
  [CATransaction setCompletionBlock:^
  {
    [self showAnimationStepWithLabels:labels buttons:buttons];
  }];
  CFTimeInterval const beginTime = CACurrentMediaTime();
  CABasicAnimation * alphaAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
  alphaAnimation.beginTime = beginTime;
  alphaAnimation.fromValue = @0.0;
  alphaAnimation.toValue = @1.0;
  [label.layer addAnimation:alphaAnimation forKey:@"alphaAnimation"];

  if (![button isEqual:self.searchButton])
  {
    CABasicAnimation * translationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.translation.y"];
    translationAnimation.beginTime = beginTime;
    translationAnimation.fromValue = @([self offsetBetweenButtons]);
    translationAnimation.toValue = @0.0;
    [button.layer addAnimation:translationAnimation forKey:@"translationAnimation"];
  }
  [CATransaction commit];
}

@end
