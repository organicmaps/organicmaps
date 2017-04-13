//
//  MPRewardedVideo+Internal.h
//  MoPubSDK
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPRewardedVideo.h"

@interface MPRewardedVideo (Internal)

+ (MPRewardedVideo *)sharedInstance;
- (void)startRewardedVideoConnectionWithUrl:(NSURL *)url;

@end
