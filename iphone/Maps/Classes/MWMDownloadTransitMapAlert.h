//
//  MWMGetTransitionMapAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"

@class MWMAlertEntity;

@interface MWMDownloadTransitMapAlert : MWMAlert
+ (instancetype)alert;
- (void)configureWithEntity:(MWMAlertEntity *)entity;
@end
