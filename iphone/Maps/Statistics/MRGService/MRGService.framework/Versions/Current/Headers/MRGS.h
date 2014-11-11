//  $Id: MRGS.h 5669 2014-10-21 09:39:51Z a.grachev $
//  MRGS.h
//  MRGServiceFramework
//
//  Created by AKEB on 22.09.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//
//  DEPRECATED_ATTRIBUTE
//  UNAVAILABLE_ATTRIBUTE

#ifndef MRGServiceFramework_MRGS_
#define MRGServiceFramework_MRGS_

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#define APPLICATION UIApplication
#else
#import <AppKit/AppKit.h>
#define APPLICATION NSApplication
#endif

#define MRGSArrayNS(...) [NSArray arrayWithObjects:__VA_ARGS__, nil]
#define MRGSConcat(...) [MRGSArrayNS(__VA_ARGS__) componentsJoinedByString:@""]

#define MRGSRandom(min, max) ((float)((float)rand() / (float)RAND_MAX) * max + min)
#define MRGSRandomInt(min, max) ((int)min + arc4random() % (max - min + 1))

#if TARGET_OS_IPHONE
#ifdef UI_USER_INTERFACE_IDIOM
#define _IS_IPHONE !(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
#define _IS_IPAD (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
#else
#define _IS_IPHONE YES
#define _IS_IPAD NO
#endif //UI_USER_INTERFACE_IDIOM
#endif //TARGET_OS_IPHONE

#if TARGET_IPHONE_SIMULATOR
#define _IS_SIMULATOR YES
#endif //TARGET_IPHONE_SIMULATOR

#if !(TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
#define _IS_MAC YES
#endif //!(TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)

#ifndef _IS_IPHONE
#define _IS_IPHONE NO
#endif //_IS_IPHONE

#ifndef _IS_IPAD
#define _IS_IPAD NO
#endif //_IS_IPAD

#ifndef _IS_SIMULATOR
#define _IS_SIMULATOR NO
#endif //_IS_SIMULATOR

#ifndef _IS_MAC
#define _IS_MAC NO
#endif //_IS_MAC

/** Получить текущее UNIX время
 * @return возвращает путь к песочнице приложения независимо от платформы
 */
NSString* MRGSHomeDirectory();

/** Шифрование при помощи бинарного ключа
 * @param text - строка для перевода в md5
 * @return md5 от text
 */
NSString* MRGSMD5(NSString* text);

/** Получить текущее UNIX время
 * @return текущее UNIX время
 */
NSTimeInterval MRGSTime();

/** Получить текущуюю дату
 * @param format	- формат времени например @"dd/MMM/yyyy"
 * @param time		- UNIX время"
 * @return текущую дату
 */
NSString* MRGSDate(NSString* format, int time);

#endif
