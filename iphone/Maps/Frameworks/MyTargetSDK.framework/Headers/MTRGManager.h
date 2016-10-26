//
//  MTRGManager.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 18.09.15.
//  Copyright © 2015 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface MTRGManager : NSObject

+ (void)setLoggingEnabled:(BOOL)loggingEnabled;

+ (BOOL)loggingEnabled;

+ (NSDictionary *)getFingerprintParams;

+ (void)trackUrl:(NSString *)trackingUrl;

@end

