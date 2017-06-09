//
//  MTRGNativePromoBanner.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 17.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MyTargetSDK/MTRGImageData.h>
#import <MyTargetSDK/MTRGNavigationType.h>
#import <MyTargetSDK/MTRGNativePromoCard.h>

@interface MTRGNativePromoBanner : NSObject

@property(nonatomic, copy) NSString *advertisingLabel;
@property(nonatomic, copy) NSString *ageRestrictions;
@property(nonatomic, copy) NSString *title;
@property(nonatomic, copy) NSString *descriptionText;
@property(nonatomic, copy) NSString *disclaimer;
@property(nonatomic, copy) NSString *category;
@property(nonatomic, copy) NSString *subcategory;
@property(nonatomic, copy) NSString *domain;
@property(nonatomic, copy) NSString *ctaText;
@property(nonatomic) NSNumber *rating;
@property(nonatomic) NSUInteger votes;
@property(nonatomic) MTRGNavigationType navigationType;
@property(nonatomic) MTRGImageData *icon;
@property(nonatomic) MTRGImageData *image;
@property(nonatomic) NSArray<MTRGNativePromoCard *> *cards;

@end
