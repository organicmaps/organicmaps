//
//  MRADBanner.h
//  MRAdService
//
//  Created by Aleksandr Karimov on 26.12.13.
//  Copyright (c) 2013 MailRu. All rights reserved.
//

#import "MRADBanner.h"

@class MRADSectionIcon;

/*
 Баннер витрины
 */
@interface MRADShowcaseBanner : MRADBanner

/**
 *  Иконка баннера
 */
@property (nonatomic, readonly, copy) NSString *icon;

/**
 *  Retina-иконка баннера
 */
@property (nonatomic, readonly, copy) NSString *iconHD;

/**
 *  Флаг, указывающий есть ли уведомление для этого баннера
 */
@property (nonatomic, readonly) BOOL hasNotification;

/**
 *  Флаг, указывающий является ли баннер основным (false) или дополнительным (true)
 */
@property (nonatomic, readonly) BOOL isSubItem;


@property (nonatomic, readonly, copy) NSString *status;
@property (nonatomic, readonly, copy) NSString *labelType;
@property (nonatomic, readonly, copy) NSString *bubbleId;
@property (nonatomic, readonly) BOOL isMain;
@property (nonatomic, readonly) BOOL requireCategoryHighlight;
@property (nonatomic, readonly) BOOL isHighlighted;
@property (nonatomic, readonly) BOOL isBanner;
@property (nonatomic, readonly) BOOL isWifiRequired;
@property (nonatomic, readonly) NSUInteger votes;
@property (nonatomic, readonly) CGFloat rating;
@property (nonatomic, readonly, copy) NSString *paidType;
@property (nonatomic, readonly, copy) NSNumber *coins;
@property (nonatomic, readonly, copy) NSString *coinsIcon;
@property (nonatomic, readonly, copy) NSString *coinsIconHd;
@property (nonatomic, readonly, copy) NSString *coinsIconTextcolor;
@property (nonatomic, readonly, copy) NSString *coinsIconBgcolor;

- (id)initWithCoder:(NSCoder *)coder;

- (void)encodeWithCoder:(NSCoder *)coder;

- (id)copyWithZone:(NSZone *)zone;


@end
