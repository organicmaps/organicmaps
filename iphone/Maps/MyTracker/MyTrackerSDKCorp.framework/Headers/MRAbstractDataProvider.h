//
//  MRAbstractDataProvider.h
//  myTrackerSDKCorp 1.3.2
//
//  Created by Igor Glotov on 23.07.14.
//  Copyright © 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MRJsonBuilder;

@interface MRAbstractDataProvider : NSObject


- (void)collectData;

- (NSDictionary *)data;

- (void)addParam:(NSString *)value forKey:(NSString *)key;
- (void) removeParamForKey:(NSString*)key;

- (BOOL)hasData;

- (NSUInteger)dataCount;

- (void)putDataToBuilder:(MRJsonBuilder *)builder;
@end
