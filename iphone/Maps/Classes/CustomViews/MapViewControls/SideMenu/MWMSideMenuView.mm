//
//  MWMSideMenuView.m
//  Maps
//
//  Created by Ilya Grechuhin on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMMapViewControlsCommon.h"
#import "MWMSideMenuDownloadBadge.h"
#import "MWMSideMenuView.h"
#import "UIKitCategories.h"

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
  self.buttons = @[self.shareLocationButton, self.settingsButton, self.downloadMapsButton, self.bookmarksButton, self.searchButton];
  self.labels = @[self.shareLocationLabel, self.settingsLabel, self.downloadMapsLabel, self.bookmarksLabel, self.searchLabel];
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
  [self updateMenuBackground];
  [self showAnimation];
  [self setNeedsLayout];
}

- (CGFloat)offsetBetweenButtons
{
  CGFloat offset = 66.0;
  CGFloat const menuWidth = self.size.width;
  CGFloat const menuHeight = self.size.height;
  if (menuWidth > menuHeight)
  {
    CGFloat const statusBarHeight = [UIApplication sharedApplication].statusBarFrame.size.height;
    NSUInteger const buttonsCount = self.buttons.count;
    CGFloat const buttonHeight = self.searchButton.height;
    CGFloat const buttonCenterOffsetToBounds = 0.5 * buttonHeight + 2.0 * kViewControlsOffsetToBounds;
    offset = MIN(offset, (menuHeight - 0.5 * statusBarHeight - 2.0 * (buttonCenterOffsetToBounds - 0.5 * buttonHeight) - buttonsCount*buttonHeight) / (buttonsCount - 1) + buttonHeight);
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
  buttonCenter.y -= offsetBetweenButtons;
  self.bookmarksButton.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.downloadMapsButton.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.settingsButton.center = buttonCenter;
  buttonCenter.y -= offsetBetweenButtons;
  self.shareLocationButton.center = buttonCenter;

  self.searchLabel.height = self.searchButton.height;
  self.bookmarksLabel.height = self.bookmarksButton.height;
  self.downloadMapsLabel.height = self.downloadMapsButton.height;
  self.settingsLabel.height = self.settingsButton.height;
  self.shareLocationLabel.height = self.shareLocationButton.height;
  
  [self.shareLocationLabel sizeToFit];
  [self.settingsLabel sizeToFit];
  [self.downloadMapsLabel sizeToFit];
  [self.bookmarksLabel sizeToFit];
  [self.searchLabel sizeToFit];
  
  CGFloat const labelWidth = 0.5 * boundsSize.width;
  self.shareLocationLabel.width = MIN(labelWidth, self.shareLocationLabel.width);
  self.settingsLabel.width = MIN(labelWidth, self.settingsLabel.width);
  self.downloadMapsLabel.width = MIN(labelWidth, self.downloadMapsLabel.width);
  self.bookmarksLabel.width = MIN(labelWidth, self.bookmarksLabel.width);
  
  self.searchLabel.midY = self.searchButton.midY;
  self.bookmarksLabel.midY = self.bookmarksButton.midY;
  self.downloadMapsLabel.midY = self.downloadMapsButton.midY;
  self.settingsLabel.midY = self.settingsButton.midY;
  self.shareLocationLabel.midY = self.shareLocationButton.midY;

  CGFloat const rightBound = self.shareLocationButton.minX - offsetLabelToButton;
  self.shareLocationLabel.maxX = rightBound;
  self.settingsLabel.maxX = rightBound;
  self.downloadMapsLabel.maxX = rightBound;
  self.bookmarksLabel.maxX = rightBound;
  self.searchLabel.maxX = rightBound;
  
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

- (void)showAnimation
{
  [self.labels enumerateObjectsUsingBlock:^(UIButton * label, NSUInteger idx, BOOL * stop)
   {
     label.alpha = 0.0;
   }];

  [self.buttons enumerateObjectsUsingBlock:^(UIButton * button, NSUInteger idx, BOOL * stop)
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

#pragma mark - Properties

- (void)setDownloadBadge:(MWMSideMenuDownloadBadge *)downloadBadge
{
  _downloadBadge = downloadBadge;
  if (![downloadBadge.superview isEqual:self.downloadMapsButton])
    [self.downloadMapsButton addSubview:downloadBadge];
}

@end
