//
//  MPRewardedVideoConnection.m
//  MoPubSDK

//  Copyright © 2016 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPRewardedVideoConnection.h"

static const NSTimeInterval kMaximumRequestRetryInterval = 900.0; // 15 mins
static const NSTimeInterval kMinimumRequestRetryInterval = 5.0;
static const NSTimeInterval kMaximumBackoffTime = 60.0;
static const CGFloat kRetryIntervalBackoffMultiplier = 2.0;

@interface MPRewardedVideoConnection()

@property (nonatomic) NSURLConnection *connection;
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
    NSURLRequest *request = [NSURLRequest requestWithURL:self.url];
    [self.connection cancel];
    self.connection = [NSURLConnection connectionWithRequest:request delegate:self];
}

- (void)retryRewardedVideoCompletionRequest
{
    NSTimeInterval retryInterval = [self backoffTime:self.retryCount];

    self.accumulatedRetryInterval += retryInterval;

    if (self.accumulatedRetryInterval < kMaximumRequestRetryInterval) {
        [self performSelector:@selector(sendRewardedVideoCompletionRequest) withObject:nil afterDelay:retryInterval];
    } else {
        [self.delegate rewardedVideoConnectionCompleted:self url:self.url];
        [self.connection cancel];
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

#pragma mark - <NSURLConnectionDataDelegate>

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    if (error.code == NSURLErrorTimedOut ||
        error.code == NSURLErrorNetworkConnectionLost ||
        error.code == NSURLErrorNotConnectedToInternet) {
        [self retryRewardedVideoCompletionRequest];
    } else {
        [self.delegate rewardedVideoConnectionCompleted:self url:self.url];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
    // only retry on 5xx
    if (statusCode >= 500 && statusCode <= 599) {
        [self retryRewardedVideoCompletionRequest];
    } else {
        [self.delegate rewardedVideoConnectionCompleted:self url:self.url];
    }
}

@end
