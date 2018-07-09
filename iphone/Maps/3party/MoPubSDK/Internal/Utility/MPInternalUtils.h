//
//  MPInternalUtils.h
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

#define SUPPRESS_PERFORM_SELECTOR_LEAK_WARNING(code)                        \
    _Pragma("clang diagnostic push")                                        \
    _Pragma("clang diagnostic ignored \"-Warc-performSelector-leaks\"")     \
    code;                                                                   \
    _Pragma("clang diagnostic pop")                                         \

@interface MPInternalUtils : NSObject

@end

@interface NSMutableDictionary (MPInternalUtils)

- (void)mp_safeSetObject:(id)obj forKey:(id<NSCopying>)key;
- (void)mp_safeSetObject:(id)obj forKey:(id<NSCopying>)key withDefault:(id)defaultObj;

@end
