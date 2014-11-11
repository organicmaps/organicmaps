//  $Id: NSDictionary+MRGS.h 5120 2014-08-21 13:49:50Z a.grachev $
//
//  NSDictionary+MRGS.h
//  MRGServiceFramework
//
//  Created by AKEB on 03.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

void MRGS_useMyLib_NSDictionary();

/** Расширение класса NSDictionary для HTTP запросов
 *
 */
@interface NSDictionary (MRGS)

/** Переделываем NSDictionary в NSString запрос для HTTP
 * @see MRGS_formatForHTTPUsingEncoding: formatForHTTPUsingEncoding:ordering:
 * @return Возвращает строку для подстановки в GET запрос
 */
- (NSString*)MRGS_formatForHTTP;

/** Переделываем NSDictionary в NSString запрос для HTTP
 * @param inEncoding Кодировка параметров (NSStringEncoding)
 * @see MRGS_formatForHTTP formatForHTTPUsingEncoding:ordering:
 * @return Возвращает строку для подстановки в GET запрос
 */
- (NSString*)MRGS_formatForHTTPUsingEncoding:(NSStringEncoding)inEncoding;

/** Переделываем NSDictionary в NSString запрос для HTTP
 * @param inEncoding Кодировка параметров (NSStringEncoding)
 * @param order YES - ключи массива будут отсортированы по алфавиту
 * @see MRGS_formatForHTTPUsingEncoding: formatForHTTP
 * @return Возвращает строку для подстановки в GET запрос
 */
- (NSString*)MRGS_formatForHTTPUsingEncoding:(NSStringEncoding)inEncoding ordering:(BOOL)order;

@end
