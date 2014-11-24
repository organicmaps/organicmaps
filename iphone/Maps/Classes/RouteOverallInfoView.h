//
//  RouteOverallInfoView.h
//  Maps
//
//  Created by Timur Bernikowich on 24/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RouteOverallInfoView : UIView

@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UILabel * metricsLabel;
@property (nonatomic) UILabel * timeLeftLabel;

- (void)updateWithInfo:(NSDictionary *)info;

@end
