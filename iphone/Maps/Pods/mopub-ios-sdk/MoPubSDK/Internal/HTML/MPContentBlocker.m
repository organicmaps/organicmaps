//
//  MPContentBlocker.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPContentBlocker.h"
#import "MPAPIEndpoints.h"

@interface MPContentBlocker()
@property (class, nonatomic, readonly) NSArray<NSString *> * blockedResources;
@end

@implementation MPContentBlocker

#pragma mark - Lazy Initialized Properties

/**
 Current list of blocked resources.
 */
+ (NSArray<NSString *> *)blockedResources {
    static NSSet<NSString *> * sBlockedResources = nil;
    NSString * blockedURLString = [NSString stringWithFormat:@"http.?://%@/mraid.js", MPAPIEndpoints.baseHostname];

    if (sBlockedResources == nil) {
        sBlockedResources = [NSSet setWithObject:blockedURLString];
    } else if (![sBlockedResources containsObject:blockedURLString]) {
        sBlockedResources = [sBlockedResources setByAddingObject:blockedURLString];
    }

    return [sBlockedResources allObjects];
}

/**
 Generates a JSON block pattern from the URL resource.
 */
+ (NSDictionary *)blockPatternFromResource:(NSString *)resource {
    if (resource == nil) {
        return nil;
    }

    // See https://developer.apple.com/documentation/safariservices/creating_a_content_blocker?language=objc
    // for the specifics of the content blocking JSON structure.
    return @{ @"action": @{ @"type": @"block" },
              @"trigger": @{ @"url-filter": resource } };
}

+ (NSString *)blockedResourcesList {
    static NSString * sBlockedResourcesList = nil;
    static NSInteger sBlockedResourcesListCount = 0;

    // Update the blocked resources string if:
    // - the string @c sBlockedResourcesList has not been initialized
    // - the count for @c blockedResources (stored in @c sBlockedResourcesListCount) has changed
    if (sBlockedResourcesList == nil || sBlockedResourcesListCount != MPContentBlocker.blockedResources.count) {
        // Update present blocked resources count to new value
        sBlockedResourcesListCount = MPContentBlocker.blockedResources.count;

        // Aggregate all resource patterns to block into a single JSON structure.
        NSMutableArray * patterns = [NSMutableArray arrayWithCapacity:MPContentBlocker.blockedResources.count];
        [MPContentBlocker.blockedResources enumerateObjectsUsingBlock:^(NSString * resource, NSUInteger idx, BOOL * _Nonnull stop) {
            NSDictionary * blockPattern = [MPContentBlocker blockPatternFromResource:resource];
            if (blockPattern != nil) {
                [patterns addObject:blockPattern];
            }
        }];

        // Generate a JSON string.
        NSError * error = nil;
        NSData * jsonData = [NSJSONSerialization dataWithJSONObject:patterns options:0 error:&error];
        sBlockedResourcesList = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    }

    return sBlockedResourcesList;
}

@end
