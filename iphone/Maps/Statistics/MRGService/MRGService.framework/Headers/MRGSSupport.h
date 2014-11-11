//  $Id: MRGSSupport.h 5672 2014-10-21 10:50:20Z a.grachev $
//
//  MRGSSupport.h
//  MRGServiceFramework
//
//  Created by Yuriy Lisenkov on 17.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSSupport_
#define MRGServiceFramework_MRGSSupport_

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#import <Foundation/Foundation.h>

/** Класс для работы со службой поддержки в MRGS. */
@interface MRGSSupport : NSObject

/** отправка письма в службу поддержки
 * @param email - email пользователя
 * @param subject - краткое описание
 * @param body - описание проблемы
 */
+ (void)sendWithEmail:(NSString*)email andSubject:(NSString*)subject andBody:(NSString*)body;

/** отправка письма в службу поддержки
 * @param params - данные для отправки в службу поддержки 
 */
+ (void)sendWithParams:(NSDictionary*)params;

@end

#endif
