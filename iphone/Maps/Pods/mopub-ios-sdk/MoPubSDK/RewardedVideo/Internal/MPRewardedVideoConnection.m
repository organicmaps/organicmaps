//
//  MPRewardedVideoConnection.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

//  Copyright © 2016 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPRewardedVideoConnection.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"

static const NSTimeInterval kMaximumRequestRetryInterval = 900.0; // 15 mins
static const NSTimeInterval kMinimumRequestRetryInterval = 5.0;
static const NSTimeInterval kMaximumBackoffTime = 60.0;
static const CGFloat kRetryIntervalBackoffMultiplier = 2.0;

@interface MPRewardedVideoConnection()

@property (nonatomic, strong) NSURLSessionTask *task;
@property (nonatomic) NSURL *url;
@property (nonatomic) NSUInteger retryCount;
@property (nonatomic) NSTimeInterval accumulatedRetryInterval;
@property (nonatomic, weak) id<MPRewardedVideoConnectionDelegate> delegate;

@end

@implementation MPRewardedVideoConnection

- (instancetype)initWithUrl:(NSURL *)url delegate:(id<MPRewardedVideoConnectionDelegate>)delegate
{
    if (self = [super init]) {
        _url = url;
        _delegate = delegate;
    }
    return self;
}

- (void)sendRewardedVideoCompletionRequest
{
    MPURLRequest *request = [MPURLRequest requestWithURL:self.url];
    [self.task cancel];

    __weak __typeof__(self) weakSelf = self;
    self.task = [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
        __typeof__(self) strongSelf = weakSelf;

        NSInteger statusCode = response.statusCode;

        // only retry on 5xx
        if (statusCode >= 500 && statusCode <= 599) {
            [strongSelf retryRewardedVideoCompletionRequest];
        } else {
            [strongSelf.delegate rewardedVideoConnectionCompleted:strongSelf url:strongSelf.url];
        }
    } errorHandler:^(NSError * _Nonnull error) {
        __typeof__(self) strongSelf = weakSelf;

        if (error.code == NSURLErrorTimedOut ||
            error.code == NSURLErrorNetworkConnectionLost ||
            error.code == NSURLErrorNotConnectedToInternet) {
            [strongSelf retryRewardedVideoCompletionRequest];
        } else {
            [strongSelf.delegate rewardedVideoConnectionCompleted:strongSelf url:strongSelf.url];
        }
    }];
}

- (void)retryRewardedVideoCompletionRequest
{
    NSTimeInterval retryInterval = [self backoffTime:self.retryCount];

    self.accumulatedRetryInterval += retryInterval;

    if (self.accumulatedRetryInterval < kMaximumRequestRetryInterval) {
        [self performSelector:@selector(sendRewardedVideoCompletionRequest) withObject:nil afterDelay:retryInterval];
    } else {
        [self.delegate rewardedVideoConnectionCompleted:self url:self.url];
        [self.task cancel];
    }
    self.retryCount++;
}

- (NSTimeInterval)backoffTime:(NSUInteger)retryCount
{
    NSTimeInterval interval = pow(kRetryIntervalBackoffMultiplier, retryCount) * kMinimumRequestRetryInterval;

    // If interval > kMaximumBackoffTime, we'll retry every kMaximumBackoffTime seconds to ensure retry happens
    // often enough.
    if (interval > kMaximumBackoffTime) {
        interval = kMaximumBackoffTime;
    }
    return interval;
}

@end
