//  $Id: MRGSSafeData.h 5667 2014-10-20 15:15:54Z a.grachev $
//
//  MRGSSafeData.h
//  MRGServiceFramework
//
//  Created by AKEB on 17.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSSafeMoney_
#define MRGServiceFramework_MRGSSafeMoney_

#import <Foundation/Foundation.h>
#import "MRGS.h"

/** Класс MRGSSafeData. 
 * 
 */
DEPRECATED_ATTRIBUTE
@interface MRGSSafeData : NSObject {
@private
    NSRecursiveLock* _locker;
    NSNumber* _data;
    int _failInt;
}

/** Устанавливаем переменную.
 *	@param value переменная
 *	@return Возращает переменную
 */
- (int)setInt:(int)value;

/** Прибавляем переменную.
 *	@param value переменная
 *	@return Возращает переменную
 */
- (int)addInt:(int)value;

/** Возвращает переменную.
 *	@return Возращает переменную
 */
- (int)intValue;

@end
#endif