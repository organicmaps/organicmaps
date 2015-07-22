//
//  MWMRoutePreview.m
//  Maps
//
//  Created by Ilya Grechuhin on 21.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRoutePreview.h"
#import "UIKitCategories.h"

@interface MWMRoutePreview ()

@property (nonatomic) CGFloat goButtonHiddenOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonVerticalOffset;
@property (weak, nonatomic) IBOutlet UIView * statusBox;
@property (weak, nonatomic) IBOutlet UIView * completeBox;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonHeight;

@end

@implementation MWMRoutePreview

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.goButtonHiddenOffset = self.goButtonVerticalOffset.constant;
  self.completeBox.hidden = YES;
}

- (IBAction)routeTypePressed:(UIButton *)sender
{
  self.pedestrian.selected = self.vehicle.selected = NO;
  sender.selected = YES;
}

#pragma mark - Properties

- (void)setShowGoButton:(BOOL)showGoButton
{
  _showGoButton = showGoButton;
  [self layoutIfNeeded];
  self.goButtonVerticalOffset.constant = showGoButton ? 0.0 : self.goButtonHiddenOffset;
  self.statusBox.hidden = YES;
  self.completeBox.hidden = NO;
  [UIView animateWithDuration:0.2 animations:^{ [self layoutIfNeeded]; }];
}

- (CGFloat)visibleHeight
{
  CGFloat height = super.visibleHeight;
  if (self.showGoButton)
    height += self.goButtonHeight.constant;
  return height;
}

@end
