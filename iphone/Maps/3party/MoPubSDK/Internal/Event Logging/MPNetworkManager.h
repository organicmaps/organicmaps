//
//  MPNetworkManager.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

// Adapted from NetworkManager in Apple's MVCNetworking sample code.

// The shared instance of this class provides a way for clients to execute network-related
// operations while minimizing the impact to tasks executing on the main thread. In order to do
// this, it manages a single dedicated networking thread.

@interface MPNetworkManager : NSObject

@property (assign, readonly) NSUInteger networkTransferOperationCount;

// Returns the network manager shared instance.
+ (instancetype)sharedNetworkManager;

// Adds the specified operation object to an internal operation queue reserved for network transfer
// operations.
//
// If the specified operation supports the `runLoopThread` property and the value of that property
// is nil, this method sets the run loop thread of the operation to the dedicated networking thread.
// Any callbacks from an asynchronous network request will then run on the networking thread's
// run loop, rather than the main run loop.
- (void)addNetworkTransferOperation:(NSOperation *)operation;

// Cancels the specified operation.
- (void)cancelOperation:(NSOperation *)operation;

@end
