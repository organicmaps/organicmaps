//
//  MPRewardedVideo+Internal.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPRewardedVideo.h"

@interface MPRewardedVideo (Internal)

+ (MPRewardedVideo *)sharedInstance;
- (void)startRewardedVideoConnectionWithUrl:(NSURL *)url;

@end
