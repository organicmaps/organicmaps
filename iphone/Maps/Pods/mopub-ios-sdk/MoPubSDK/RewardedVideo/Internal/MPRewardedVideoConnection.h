//
//  MPRewardedVideoConnection.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

@class MPRewardedVideoConnection;

@protocol MPRewardedVideoConnectionDelegate <NSObject>

- (void)rewardedVideoConnectionCompleted:(MPRewardedVideoConnection *)connection url:(NSURL *)url;

@end

@interface MPRewardedVideoConnection : NSObject

- (instancetype)initWithUrl:(NSURL *)url delegate:(id<MPRewardedVideoConnectionDelegate>)delegate;
- (void)sendRewardedVideoCompletionRequest;

@end
