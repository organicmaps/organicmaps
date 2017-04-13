//
//  MPNetworkManager.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPNetworkManager.h"

#import "MPCoreInstanceProvider.h"
#import "MPRetryingHTTPOperation.h"

static const double kNetworkThreadPriority = 0.3;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNetworkManager ()

@property (strong) NSThread *networkThread;
@property (strong, readwrite) NSOperationQueue *networkTransferQueue;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPNetworkManager

+ (instancetype)sharedNetworkManager
{
    static MPNetworkManager *sNetworkManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sNetworkManager = [[[self class] alloc] init];
        [sNetworkManager startNetworkThread];
    });
    return sNetworkManager;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _networkThread = [[NSThread alloc] initWithTarget:self selector:@selector(networkThreadMain) object:nil];
        _networkThread.name = @"com.mopub.MPNetworkManager";
        _networkThread.threadPriority = kNetworkThreadPriority;
        
        _networkTransferQueue = [[NSOperationQueue alloc] init];
        _networkTransferQueue.maxConcurrentOperationCount = 1;
    }
    return self;
}

- (void)startNetworkThread
{
    [self.networkThread start];
}

- (void)networkThreadMain
{
    NSAssert(![NSThread isMainThread], @"The network thread should not be the main thread.");
    
    @autoreleasepool {
        // Add a dummy input source to prevent the run loop from exiting immediately.
        NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
        [runLoop addPort:[NSMachPort port] forMode:NSDefaultRunLoopMode];
        [runLoop run];
    }
}

#pragma mark - Public

- (void)addNetworkTransferOperation:(NSOperation *)operation
{
    if ([operation respondsToSelector:@selector(setRunLoopThread:)]) {
        if (![(id)operation runLoopThread]) {
            [(id)operation setRunLoopThread:self.networkThread];
        }
    }
    
    [self.networkTransferQueue addOperation:operation];
}

- (void)cancelOperation:(NSOperation *)operation
{
    [operation cancel];
}

- (NSUInteger)networkTransferOperationCount
{
    return [self.networkTransferQueue operationCount];
}

@end
