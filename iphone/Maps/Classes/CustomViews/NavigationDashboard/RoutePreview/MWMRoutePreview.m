//
//  MWMRoutePreview.m
//  Maps
//
//  Created by Ilya Grechuhin on 21.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRoutePreview.h"

@implementation MWMRoutePreview

- (void)layoutSubviews
{
  self.frame = CGRectMake(0.0, 0.0, self.superview.width, 76.0);
}

- (IBAction)routeTypePressed:(UIButton *)sender
{
  self.pedestrian.selected = self.vehicle.selected = NO;
  sender.selected = YES;
}

@end
