//  $Id: MRGSJson.h 5667 2014-10-20 15:15:54Z a.grachev $
//
//  MRGSJson.h
//  MRGServiceFramework
//
//  Created by Yuriy Lisenkov on 22.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSJson_
#define MRGServiceFramework_MRGSJson_

#import <Foundation/Foundation.h>
#import "MRGS.h"

/** Класс MRGSJson. 
 * 
 */
@interface MRGSJson : NSObject

/** Возвращает объект, созданный из строки, либо ноль в случае ошибки.
 * @param string строка с данными в формате json
 * @return Возвращаемый объект может быть string, number, boolean, null, array или dictionary
 */
+ (id)objectWithString:(NSString*)string;

/** Возвращает json строку, полученную из объекта.
 * @param object из которого необходимо получить json строку
 * @return Возвращаемый string либо nil в случае ошибки
 */
+ (id)stringWithObject:(id)object;

/** Возвращает объект, созданный из строки, либо ноль в случае ошибки.
 * @param string строка с данными в формате json
 * @return Возвращаемый объект может быть string, number, boolean, null, array или dictionary
 */
- (id)objectWithString:(NSString*)string;

/** Возвращает json строку, полученную из объекта.
 * @param object из которого необходимо получить json строку
 * @return Возвращаемый string либо nil в случае ошибки
 */
- (id)stringWithObject:(id)object;

@end

#endif
