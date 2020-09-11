//
//  MPURL.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPURL.h"

@implementation MPURL

#pragma mark - Initialization

- (instancetype)initWithString:(NSString *)URLString {
    if (self = [super initWithString:URLString]) {
        _postData = [NSMutableDictionary dictionary];
    }
    return self;
}

+ (instancetype)URLWithString:(NSString *)URLString {
    return [[MPURL alloc] initWithString:URLString];
}

+ (instancetype)URLWithComponents:(NSURLComponents *)components postData:(NSDictionary<NSString *, NSObject *> *)postData {
    NSString * urlString = components.URL.absoluteString;
    MPURL * mpUrl = [[MPURL alloc] initWithString:urlString];
    [mpUrl.postData addEntriesFromDictionary:postData];

    return mpUrl;
}

#pragma mark - POST Data Convenience Getters

- (NSArray *)arrayForPOSTDataKey:(NSString *)key {
    return (NSArray *)self.postData[key];
}

- (NSDictionary *)dictionaryForPOSTDataKey:(NSString *)key {
    return (NSDictionary *)self.postData[key];
}

- (NSNumber *)numberForPOSTDataKey:(NSString *)key {
    return (NSNumber *)self.postData[key];
}

- (NSString *)stringForPOSTDataKey:(NSString *)key {
    return (NSString *)self.postData[key];
}

@end
