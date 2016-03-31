//
//  MTRGManager_Corp.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 18.09.15.
//  Copyright © 2015 Mail.ru. All rights reserved.
//

#import <MyTargetSDKCorp/MTRGManager.h>

@interface MTRGManager ()

//Отправлять все запросы на сервера my.com
+(void) setMyCom:(BOOL)useMyCom;
//Получить данные о девайсе
+(NSDictionary*)getFingerprintParams;
//Отправить статистику
+ (void)trackUrl:(NSString *)trackingUrl;

@end
