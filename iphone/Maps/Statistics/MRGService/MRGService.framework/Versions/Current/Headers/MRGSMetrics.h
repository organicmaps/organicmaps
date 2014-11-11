//  $Id: MRGSMetrics.h 5120 2014-08-21 13:49:50Z a.grachev $
//
//  MRGSMetrics.h
//  MRGServiceFramework
//
//  Created by AKEB on 05.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSMetrics_
#define MRGServiceFramework_MRGSMetrics_

#import <Foundation/Foundation.h>

/**  Класс MRGSMetrics. В инициализации не нуждается.
 *
 */
@interface MRGSMetrics : NSObject

#pragma mark -
#pragma mark МЕТОДЫ

/** отправка метрики
 * @param metricId - уникальный идентификатор метрики
 * @see addMetricWithId:andValue:
 * @see addMetricWithId:andValue:andLevel:
 * @see addMetricWithId:andValue:andLevel:andObjectId:
 */
+ (void)addMetricWithId:(int)metricId;

/** отправка метрики
 * @param metricId - уникальный идентификатор метрики
 * @param value - значение метрики
 * @see addMetricWithId:
 * @see addMetricWithId:andValue:andLevel:
 * @see addMetricWithId:andValue:andLevel:andObjectId:
 */
+ (void)addMetricWithId:(int)metricId andValue:(int)value;

/** отправка метрики
 * @param metricId - строкцйа для перевода в md5
 * @param value - значение метрики
 * @param level - значение текущего уровня
 * @see addMetricWithId:
 * @see addMetricWithId:andValue:andLevel:
 * @see addMetricWithId:andValue:andLevel:andObjectId:
 */
+ (void)addMetricWithId:(int)metricId andValue:(int)value andLevel:(int)level;

/** отправка метрики
 * @param metricId - строкцйа для перевода в md5
 * @param value - значение метрики
 * @param level - значение текущего уровня
 * @param objectId - id объекта
 * @see addMetricWithId:
 * @see addMetricWithId:andValue:
 * @see addMetricWithId:andValue:andLevel:
 */
+ (void)addMetricWithId:(int)metricId andValue:(int)value andLevel:(int)level andObjectId:(int)objectId;

@end

#endif