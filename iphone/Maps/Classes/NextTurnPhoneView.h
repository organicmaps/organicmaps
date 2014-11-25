//
//  NextTurnPhoneView.h
//  Maps
//
//  Created by Timur Bernikowich on 24/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface NextTurnPhoneView : UIView

@property (nonatomic) UIImageView * turnTypeView;
@property (nonatomic) UILabel * turnValue;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UILabel * metricsLabel;

- (void)updateWithInfo:(NSDictionary *)info;

@end