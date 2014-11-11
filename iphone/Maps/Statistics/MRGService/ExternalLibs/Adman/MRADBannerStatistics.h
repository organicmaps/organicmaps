//
// Created by a_karimov on 31.12.13.
//
// Copyright (c) 2013 AlexKar. All rights reserved.
//


#import <Foundation/Foundation.h>

/**
 *  Объект-контейнер для хранения статистики баннера
 */
@interface MRADBannerStatistics : NSObject <NSCoding>

/**
 *  Ссылка для отправки статистики
 */
@property (nonatomic, copy, readonly) NSString *urlString;

/**
 *  Тип статистики
 */
@property (nonatomic, copy, readonly) NSString *type;

@property (nonatomic, copy, readonly) NSDictionary *jsonDict;

@end