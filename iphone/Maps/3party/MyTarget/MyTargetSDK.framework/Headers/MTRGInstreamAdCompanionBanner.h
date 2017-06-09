//
//  MTRGInstreamAdCompanionBanner.h
//  MyTargetSDK
//
//  Created by Andrey Seredkin on 14.12.16.
//  Copyright © 2016 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MTRGInstreamAdCompanionBanner : NSObject

@property(nonatomic) NSUInteger width;
@property(nonatomic) NSUInteger height;
@property(nonatomic) NSUInteger assetWidth;
@property(nonatomic) NSUInteger assetHeight;
@property(nonatomic) NSUInteger expandedWidth;
@property(nonatomic) NSUInteger expandedHeight;

@property(nonatomic, copy) NSString *staticResource;
@property(nonatomic, copy) NSString *iframeResource;
@property(nonatomic, copy) NSString *htmlResource;
@property(nonatomic, copy) NSString *apiFramework;
@property(nonatomic, copy) NSString *adSlotID;
@property(nonatomic, copy) NSString *required;

@end
