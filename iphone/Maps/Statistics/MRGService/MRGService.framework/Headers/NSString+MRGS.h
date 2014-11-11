//  $Id: NSString+MRGS.h 5120 2014-08-21 13:49:50Z a.grachev $
//
//  NSString+MRGS.h
//  MRGServiceFramework
//
//  Created by AKEB on 02.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonCryptor.h>

void MRGS_useMyLib_NSString();

/** Расширение класса NSString для шифрования данных
 *
 */
@interface NSString (MRGS)

/** Шифрование при помощи строкового ключа
 * @param key строковый ключ
 * @return Зашифрованные данные
 */
- (NSData*)MRGS_encrypt:(NSString*)key;

/** Расшифровка при помощи строкового ключа
 * @param key строковый ключ
 * @return Расшифрованные данные
 */
- (NSData*)MRGS_decrypt:(NSString*)key;

/** Создает перевернутую строку
 * @param str исходная строка
 * @return Возвращает перевернутую строку
 */
+ (NSString*)MRGS_stringAsReverseString:(NSString*)str;

/** Создает md5 от строки
 * @param str исходная строка
 * @return Возвращает md5 от строки
 */
+ (NSString*)MRGS_md5:(NSString*)str;

/** Преобразует строку в md5
 * @return Возвращает md5 от строки
 */
- (NSString*)MRGS_md5;

/** Переворачиваем строку
 * @return Возвращает перевернутую строку
 */
- (NSString*)MRGS_stringAsReverseString;

/** Получить json объект из строки
 * @return Возвращает NSDictionary или NSArray представление JSON строки
 */
- (id)MRGS_JSONValue;

/** Получить json объект из строки
 * @return Возвращает NSDictionary или NSArray представление JSON строки
 * @param string json строка
 */
+ (id)MRGS_JSONValue:(NSString*)string;

@end
