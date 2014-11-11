//
//  MRGSNanigans.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 24.10.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

/** Класс для отправки событий в Nanigans. */
@interface MRGSNanigans : NSObject
/**
 *  Отправка события в статистику Nanigans.
 *
 *  @param eventType    Тип события (eventType needs to have one of the following values: user, install, purchase, visit, viral)
 *  @param name         Наименование события
 *  @param params       Дополнительные параметры
 */
+ (void)trackEventType:(NSString *)eventType name:(NSString *)name extraParams:(NSDictionary *)params;
@end
