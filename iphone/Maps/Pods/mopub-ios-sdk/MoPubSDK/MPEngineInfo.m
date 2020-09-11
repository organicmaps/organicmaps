//
//  MPEngineInfo.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPEngineInfo.h"
#import "MPAdServerKeys.h"

@implementation MPEngineInfo

- (instancetype)initWithName:(NSString *)name version:(NSString *)version {
    // Both name and version are required fields.
    if (name.length == 0 || version.length == 0) {
        return nil;
    }

    if (self = [super init]) {
        _name = name;
        _version = version;
    }

    return self;
}

+ (instancetype)named:(NSString *)name version:(NSString *)version {
    return [[MPEngineInfo alloc] initWithName:name version:version];
}

#pragma mark - NSCoding

- (instancetype)initWithCoder:(NSCoder *)coder {
    // Validate that the required parameters exist.
    NSString * name = [coder decodeObjectForKey:kSDKEngineNameKey];
    NSString * version = [coder decodeObjectForKey:kSDKEngineVersionKey];
    if (name.length == 0 || version.length == 0) {
        return nil;
    }

    return [self initWithName:name version:version];
}

- (void)encodeWithCoder:(NSCoder *)coder {
    if (self.name.length > 0) {
        [coder encodeObject:self.name forKey:kSDKEngineNameKey];
    }

    if (self.version.length > 0) {
        [coder encodeObject:self.version forKey:kSDKEngineVersionKey];
    }
}

@end
