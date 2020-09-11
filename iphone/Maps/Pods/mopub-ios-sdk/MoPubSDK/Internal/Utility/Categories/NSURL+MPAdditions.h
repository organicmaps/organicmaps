//
//  NSURL+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

typedef enum {
    MPMoPubHostCommandUnrecognized,
    MPMoPubHostCommandClose,
    MPMoPubHostCommandFinishLoad,
    MPMoPubHostCommandFailLoad,
    MPMoPubHostCommandPrecacheComplete,
    MPMoPubHostCommandRewardedVideoEnded
} MPMoPubHostCommand;

typedef enum {
    MPMoPubShareHostCommandTweet,
    MPMoPubShareHostCommandUnrecognized
} MPMoPubShareHostCommand;

@interface NSURL (MPAdditions)

- (NSString *)mp_queryParameterForKey:(NSString *)key;
- (NSArray *)mp_queryParametersForKey:(NSString *)key;
- (NSDictionary *)mp_queryAsDictionary;
- (BOOL)mp_isSafeForLoadingWithoutUserAction;
- (BOOL)mp_isMoPubScheme;
- (MPMoPubHostCommand)mp_mopubHostCommand;
- (BOOL)mp_isMoPubShareScheme;
- (MPMoPubShareHostCommand)mp_MoPubShareHostCommand;

@end
