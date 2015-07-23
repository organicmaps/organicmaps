//
//  MWMRoutePreview.m
//  Maps
//
//  Created by Ilya Grechuhin on 21.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationDashboardEntity.h"
#import "MWMRoutePreview.h"
#import "TimeUtils.h"
#import "UIKitCategories.h"

@interface MWMRoutePreview ()

@property (nonatomic) CGFloat goButtonHiddenOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonVerticalOffset;
@property (weak, nonatomic) IBOutlet UIView * statusBox;
@property (weak, nonatomic) IBOutlet UIView * completeBox;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonHeight;

@property (nonatomic) BOOL showGoButton;

@end

@implementation MWMRoutePreview

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.goButtonHiddenOffset = self.goButtonVerticalOffset.constant;
  [self statePlaning];
}

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity
{
  self.timeLabel.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(entity.timeToTarget)];
  self.distanceLabel.text = [NSString stringWithFormat:@"%@ %@", entity.targetDistance, entity.targetUnits];
}

- (void)statePlaning
{
  self.showGoButton = NO;
  self.statusBox.hidden = NO;
  self.completeBox.hidden = YES;
  self.spinner.hidden = NO;
  self.cancelButton.hidden = YES;

  NSUInteger const capacity = 12;
  NSMutableArray * images = [NSMutableArray arrayWithCapacity:capacity];
  for (int i = 1; i != capacity; ++i)
    [images addObject:[UIImage imageNamed:[NSString stringWithFormat:@"ic_spinner_close_%@", @(i)]]];

  self.spinner.imageView.animationImages = images.copy;
  self.spinner.imageView.animationDuration = .5;
  [self.spinner.imageView startAnimating];
}

- (void)showGoButtonAnimated:(BOOL)show
{
  [self layoutIfNeeded];
  self.showGoButton = show;
  [UIView animateWithDuration:0.2 animations:^{ [self layoutIfNeeded]; }];
}

#pragma mark - Properties

- (void)setShowGoButton:(BOOL)showGoButton
{
  _showGoButton = showGoButton;
  self.goButtonVerticalOffset.constant = showGoButton ? 0.0 : self.goButtonHiddenOffset;
  self.statusBox.hidden = YES;
  self.completeBox.hidden = NO;
  self.spinner.hidden = YES;
  self.cancelButton.hidden = NO;
}

- (CGFloat)visibleHeight
{
  CGFloat height = super.visibleHeight;
  if (self.showGoButton)
    height += self.goButtonHeight.constant;
  return height;
}

@end
