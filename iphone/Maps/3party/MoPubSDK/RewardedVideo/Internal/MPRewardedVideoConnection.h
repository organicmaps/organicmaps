//
//  MPRewardedVideoConnection.h
//  MoPubSDK
//  Copyright © 2016 MoPub. All rights reserved.
//

@class MPRewardedVideoConnection;

@protocol MPRewardedVideoConnectionDelegate <NSObject>

- (void)rewardedVideoConnectionCompleted:(MPRewardedVideoConnection *)connection url:(NSURL *)url;

@end

@interface MPRewardedVideoConnection : NSObject

- (instancetype)initWithUrl:(NSURL *)url delegate:(id<MPRewardedVideoConnectionDelegate>)delegate;
- (void)sendRewardedVideoCompletionRequest;

@end
