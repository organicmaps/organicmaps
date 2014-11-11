//	$Id: MRGSLogs.h 5602 2014-10-14 08:06:47Z a.grachev $
//  MRGSLog.h
//  MRGServiceFramework
//
//  Created by AKEB on 23.09.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

#define MRGSLogS(...) [MRGSLogs MRGSLogS:[[NSString stringWithFormat:@"%s line:%i ", __PRETTY_FUNCTION__, __LINE__] stringByAppendingFormat:__VA_ARGS__]

/** Класс для вывода логов в консоль. */
@interface MRGSLogs : NSObject

/**
*  Вывод в консоль строки и отправка ее на сервер.
*
*  @param log  Строка, которую нужно вывести в лог и отправить.
*/
+ (void)MRGSLogS:(NSString*)log;

@end
