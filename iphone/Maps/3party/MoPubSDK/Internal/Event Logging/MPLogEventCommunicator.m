//
//  MPLogEventCommunicator.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPLogEventCommunicator.h"
#import "MPLogging.h"
#import "MPLogEvent.h"
#import "MPRetryingHTTPOperation.h"
#import "MPNetworkManager.h"
#import "MPCoreInstanceProvider.h"

static NSString *const kAnalyticsURL = @"https://analytics.mopub.com/i/jot/exchange_client_event";

static const NSInteger MAX_CONCURRENT_CONNECTIONS = 1;

@interface MPLogEventCommunicator ()

#if !OS_OBJECT_USE_OBJC
@property (nonatomic, assign) dispatch_queue_t eventProcessingQueue;
#else
@property (nonatomic, strong) dispatch_queue_t eventProcessingQueue;
#endif

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPLogEventCommunicator

- (instancetype)init
{
    if (self = [super init]) {
        _eventProcessingQueue = dispatch_queue_create("com.mopub.eventProcessingQueue", DISPATCH_QUEUE_SERIAL);
    }

    return self;
}

- (void)dealloc
{
#if !OS_OBJECT_USE_OBJC
    dispatch_release(_eventProcessingQueue);
#endif
}

- (void)sendEvents:(NSArray *)events
{
    if (events && [events count]) {
        dispatch_async(self.eventProcessingQueue, ^{
            NSURLRequest *request = [self buildRequestWithEvents:events];
            MPRetryingHTTPOperation *operation = [[MPRetryingHTTPOperation alloc] initWithRequest:request];
            [[[MPCoreInstanceProvider sharedProvider] sharedNetworkManager] addNetworkTransferOperation:operation];
        });
    }
}

- (BOOL)isOverLimit
{
    if ([[[MPCoreInstanceProvider sharedProvider] sharedNetworkManager] networkTransferOperationCount] >= MAX_CONCURRENT_CONNECTIONS) {
        return YES;
    }
    return NO;
}

- (NSURLRequest *)buildRequestWithEvents:(NSArray *)events
{
    NSURL *URL = [NSURL URLWithString:kAnalyticsURL];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
    [request setHTTPMethod:@"POST"];

    NSString *POSTBodyString = [self makeParamStringForEvents:events];
    [request setHTTPBody:[POSTBodyString dataUsingEncoding:NSUTF8StringEncoding]];

    return request;
}

- (NSString *)makeParamStringForEvents:(NSArray *)events
{
    NSMutableArray *serializedEvents = [[NSMutableArray alloc] init];
    for (id event in events) {
        [serializedEvents addObject:[event asDictionary]];
    }
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:serializedEvents options:0 error:nil];

    NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    NSString *paramString = [NSString stringWithFormat:@"log=%@", [jsonString mp_URLEncodedString]];

    return paramString;
}

@end
