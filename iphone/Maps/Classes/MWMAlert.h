//
//  MWMAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, MWMAlertType) {
  MWMAlertTypeDownloadAllMaps,
  MWMAlertTypeDownloadTransitMap,
  MWMAlertTypeRouteNotFoundDefault
};

@class MWMAlertViewController;
@class MWMAlertEntity;

@interface MWMAlert : UIView
@property (nonatomic, weak) MWMAlertViewController *alertController;

+ (MWMAlert *)alertWithType:(MWMAlertType)type;
- (void)configureWithEntity:(MWMAlertEntity *)entity;

@end
