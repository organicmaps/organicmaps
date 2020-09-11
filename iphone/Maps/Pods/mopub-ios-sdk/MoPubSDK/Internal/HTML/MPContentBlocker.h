//
//  MPContentBlocker.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MPContentBlocker : NSObject
/**
 Blocked resources for use with @c WKContentRuleListStore.
 */
@property (class, nonatomic, readonly, nullable) NSString * blockedResourcesList;
@end

NS_ASSUME_NONNULL_END
