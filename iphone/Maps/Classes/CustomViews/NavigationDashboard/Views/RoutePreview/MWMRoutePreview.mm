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
#import "UIColor+MapsMeColor.h"
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
  NSString * arriveStr = [NSDateFormatter localizedStringFromDate:[[NSDate date]
                                           dateByAddingTimeInterval:entity.timeToTarget]
                                                          dateStyle:NSDateFormatterNoStyle
                                                          timeStyle:NSDateFormatterShortStyle];
  self.arrivalsLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"routing_arrive"), arriveStr];
}

- (void)remove
{
  [super remove];
  self.pedestrian.enabled = YES;
  self.vehicle.enabled = YES;
}

- (void)statePlaning
{
  self.showGoButton = NO;
  self.statusBox.hidden = NO;
  self.completeBox.hidden = YES;
  self.spinner.hidden = NO;
  self.cancelButton.hidden = YES;
  self.status.text = L(@"routing_planning");
  self.status.textColor = UIColor.blackHintText;

  NSUInteger const capacity = 12;
  NSMutableArray * images = [NSMutableArray arrayWithCapacity:capacity];
  for (int i = 0; i < capacity; ++i)
    images[i] = [UIImage imageNamed:[NSString stringWithFormat:@"ic_spinner_close_%@", @(i + 1)]];

  self.spinner.imageView.animationImages = images;
  [self.spinner.imageView startAnimating];
}

- (void)stateError
{
  self.spinner.hidden = YES;
  self.cancelButton.hidden = NO;
  self.status.text = L(@"routing_planning_error");
  self.status.textColor = UIColor.red;
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
