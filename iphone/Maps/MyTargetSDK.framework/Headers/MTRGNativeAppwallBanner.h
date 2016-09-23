//
//  MTRGNativeAppwallBanner.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 13.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGImageData.h>


@interface MTRGNativeAppwallBanner : NSObject

@property(nonatomic, copy) NSString *status;
@property(nonatomic, copy) NSString *title;
@property(nonatomic, copy) NSString *descriptionText;
@property(nonatomic, copy) NSString *paidType;
@property(nonatomic, copy) NSString *mrgsId;
@property(nonatomic) BOOL hasNotification;
@property(nonatomic) BOOL subitem;
@property(nonatomic) BOOL isAppInstalled;
@property(nonatomic) BOOL main;
@property(nonatomic) BOOL requireCategoryHighlight;
@property(nonatomic) BOOL banner;
@property(nonatomic) BOOL requireWifi;
@property(nonatomic) NSNumber *rating;
@property(nonatomic) NSUInteger votes;
@property(nonatomic) NSUInteger coins;
@property(nonatomic) UIColor *coinsBgColor;
@property(nonatomic) UIColor *coinsTextColor;
@property(nonatomic) MTRGImageData *icon;
@property(nonatomic) MTRGImageData *statusImage;
@property(nonatomic) MTRGImageData *coinsIcon;
@property(nonatomic) MTRGImageData *crossNotifIcon;
@property(nonatomic) MTRGImageData *bubbleIcon;
@property(nonatomic) MTRGImageData *gotoAppIcon;
@property(nonatomic) MTRGImageData *itemHighlightIcon;
@end