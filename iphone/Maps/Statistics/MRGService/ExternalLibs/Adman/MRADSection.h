//
//  MRADSection.h
//  MRAdService
//
//  Created by Aleksandr Karimov on 26.12.13.
//  Copyright (c) 2013 MailRu. All rights reserved.
//

#import <Foundation/Foundation.h>
@class MRADSectionIcon;

#pragma mark MRADSection class definition

/**
 *  Секция баннеров (секция объявлений, содержащая несколько баннеров одного типа)
 */
@interface MRADSection : NSObject <NSCoding>

/**
 *  Название секции
 */
@property (nonatomic, readonly, copy) NSString *title;

/**
 *  Баннеры секции
 *  @see MRADBanner
 */
@property (nonatomic, readonly, copy) NSArray *banners;

/**
 *  Флаг, указывающий есть ли уведомления в секции
 *  @discussion Если хотя бы один баннер секции имеет уведомление, то содержит true, иначе если все баннеры секции не содержат уведомлений, содержит false
 */
@property (nonatomic, readonly) BOOL hasNotification;

/**
 *  Иконка секции
 */
@property (nonatomic, readonly, copy) NSString *icon;

/**
 *  Retina-иконка секции
 */
@property (nonatomic, readonly, copy) NSString *iconHD;

/**
 *  Иконка для отобржения наличния уведомлений
 */
@property (nonatomic, readonly, copy) NSString *bubbleIcon;

/**
 *  Retina-иконка для отобржения наличния уведомлений
 */
@property (nonatomic, readonly, copy) NSString *bubbleIconHD;

/**
 *  Иконка метки секции
 */
@property (nonatomic, readonly, copy) NSString *labelIcon;

/**
 *  Retina-иконка метки секции
 */
@property (nonatomic, readonly, copy) NSString *labelIconHD;

/**
 *  Иконки сатусов объявлений (hit, new и т.п.)
 */
@property (nonatomic, readonly, copy) NSArray *statusIcons;

/**
 *  Порядковый номер секции
 */
@property (nonatomic, readonly) NSInteger index;

/**
 *  Формат секции
 */
@property (nonatomic, readonly, copy) NSString *format;

@property (nonatomic, copy, readonly) NSDictionary *jsonDict;

@end

