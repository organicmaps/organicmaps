//
//  CJSONSerialization.h
//  TouchJSON
//
//  Created by Jonathan Wight on 03/04/11.
//  Copyright 2011 toxicsoftware.com. All rights reserved.
//

#import <Foundation/Foundation.h>

enum {
    kCJSONReadingMutableContainers = 0x1,
    kCJSONReadingMutableLeaves = 0x2,
    kCJSONReadingAllowFragments = 0x04,
};
typedef NSUInteger EJSONReadingOptions;

enum {
    kCJJSONWritingPrettyPrinted = 0x1
};
typedef NSUInteger EJSONWritingOptions;


@interface CJSONSerialization : NSObject {

}

+ (BOOL)isValidJSONObject:(id)obj;
+ (NSData *)dataWithJSONObject:(id)obj options:(EJSONWritingOptions)opt error:(NSError **)error;
+ (id)JSONObjectWithData:(NSData *)data options:(EJSONReadingOptions)opt error:(NSError **)error;
+ (NSInteger)writeJSONObject:(id)obj toStream:(NSOutputStream *)stream options:(EJSONWritingOptions)opt error:(NSError **)error;
+ (id)JSONObjectWithStream:(NSInputStream *)stream options:(EJSONReadingOptions)opt error:(NSError **)error;

@end
