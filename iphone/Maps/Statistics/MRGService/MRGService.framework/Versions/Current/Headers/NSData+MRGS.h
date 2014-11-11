//  $Id: NSData+MRGS.h 5120 2014-08-21 13:49:50Z a.grachev $
//
//  NSData+MRGS.h
//  MRGServiceFramework
//
//  Created by AKEB on 02.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonCryptor.h>

void MRGS_useMyLib_NSData();

NSData* MRGS_sha1(NSData* bytes);
NSData* MRGS_sha256(NSData* bytes);

NSData* MRGS_cipher(NSData* key,
                    NSData* value,
                    CCOperation operation,
                    CCOptions options,
                    NSMutableData* output);

/** Расширение класса NSData для шифрования данных
 *
 */
@interface NSData (MRGS)

/** Шифрование при помощи бинарного ключа
 * @param key бинарный ключ
 * @return Зашифрованные данные
 */
- (NSData*)MRGS_encrypt:(NSData*)key;

/** Шифрование при помощи строкового ключа
 * @param key строковый ключ
 * @return Зашифрованные данные
 */
- (NSData*)MRGS_encryptWithString:(NSString*)key;

/** Расшифровка при помощи бинарного ключа
 * @param key бинарный ключ
 * @return Расшифрованные данные
 */
- (NSData*)MRGS_decrypt:(NSData*)key;

/** Расшифровка при помощи строкового ключа
 * @param key строковый ключ
 * @return Расшифрованные данные
 */
- (NSData*)MRGS_decryptWithString:(NSString*)key;

/** Описание бинарных данных в виде HEX. Без пробелов
 * 
 * @return Строка в виде HEX
 */
- (NSString*)MRGS_descriptionAsHex;

/** Описание бинарных данных в виде строки
 * @param encodeNonPrintable Выводить только печатаемые символы?
 * @see MRGS_descriptionAsCharacters
 * @return Строка
 */
- (NSString*)MRGS_descriptionAsCharacters:(BOOL)encodeNonPrintable;

/** Описание бинарных данных в виде строки
 * @see MRGS_descriptionAsCharacters:
 * @return Строка состоящая только из печтаных символов
 */
- (NSString*)MRGS_descriptionAsCharacters;

/** Переобразует base64 в NSData
 * @param string Строка base64
 * @see MRGS_initWithBase64EncodedString:
 * @return NSData из строки base64
 */
+ (NSData*)MRGS_dataWithBase64EncodedString:(NSString*)string;

/** Переобразует base64 в NSData
 * @param string Строка base64
 * @see MRGS_dataWithBase64EncodedString:
 * @return NSData из строки base64
 */
- (id)MRGS_initWithBase64EncodedString:(NSString*)string;

/** Переобразует NSData в base64
 * @see MRGS_base64EncodingWithLineLength:
 * @return строка base64
 */
- (NSString*)MRGS_base64Encoding;

/** Переобразует NSData в base64
 * @param lineLength длина блоков
 * @see MRGS_base64Encoding
 * @return строка base64
 */
- (NSString*)MRGS_base64EncodingWithLineLength:(unsigned int)lineLength;

@end
