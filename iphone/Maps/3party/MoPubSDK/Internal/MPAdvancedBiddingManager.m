//
//  MPAdvancedBiddingManager.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPAdvancedBiddingManager.h"
#import "MPLogging.h"

// JSON constants
static NSString const * kTokenKey = @"token";

@interface MPAdvancedBiddingManager()

// Dictionary of Advanced Bidding network name to instance of that Advanced Bidder.
@property (nonatomic, strong) NSMutableDictionary<NSString *, id<MPAdvancedBidder>> * bidders;

// Advanced Bidder initialization queue.
@property (nonatomic, strong) dispatch_queue_t queue;

@end

@implementation MPAdvancedBiddingManager

#pragma mark - Initialization

+ (MPAdvancedBiddingManager *)sharedManager {
    static MPAdvancedBiddingManager * sharedMyManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedMyManager = [[self alloc] init];
    });
    return sharedMyManager;
}

- (instancetype)init {
    if (self = [super init]) {
        _advancedBiddingEnabled = YES;
        _bidders = [NSMutableDictionary dictionary];
        _queue = dispatch_queue_create("Advanced Bidder Initialization Queue", NULL);
    }

    return self;
}

#pragma mark - Bidders

- (NSString *)bidderTokensJson {
    // No bidders.
    if (self.bidders.count == 0) {
        return nil;
    }

    // Advanced Bidding is not enabled.
    if (!self.advancedBiddingEnabled) {
        return nil;
    }

    // Generate the JSON dictionary for all participating bidders.
    NSMutableDictionary * tokens = [NSMutableDictionary dictionary];
    [self.bidders enumerateKeysAndObjectsUsingBlock:^(NSString * network, id<MPAdvancedBidder> bidder, BOOL * stop) {
        tokens[network] = @{ kTokenKey: bidder.token };
    }];

    // Serialize the JSON dictionary into a JSON string.
    NSError * error = nil;
    NSData * jsonData = [NSJSONSerialization dataWithJSONObject:tokens options:0 error:&error];
    if (jsonData == nil) {
        MPLogError(@"Failed to generate a JSON string from\n%@\nReason: %@", tokens, error.localizedDescription);
        return nil;
    }

    return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

- (void)initializeBidders:(NSArray<Class<MPAdvancedBidder>> * _Nonnull)bidders complete:(void(^_Nullable)(void))complete {
    // No bidders to initialize, complete immediately
    if (bidders.count == 0) {
        if (complete) {
            complete();
        }
        return;
    }

    // Asynchronous dispatch the initialization as it may take some time.
    __weak __typeof__(self) weakSelf = self;
    dispatch_async(self.queue, ^{
        __typeof__(self) strongSelf = weakSelf;
        if (strongSelf != nil) {
            for (Class<MPAdvancedBidder> advancedBidderClass in bidders) {
                // Create an instance of the Advanced Bidder
                id<MPAdvancedBidder> advancedBidder = (id<MPAdvancedBidder>)[[[advancedBidderClass class] alloc] init];
                NSString * network = advancedBidder.creativeNetworkName;

                // Verify that the Advanced Bidder has a creative network name and that it's
                // not already created.
                if (network != nil && strongSelf.bidders[network] == nil) {
                    strongSelf.bidders[network] = advancedBidder;
                }
            }
        }

        // Notify completion block handler.
        if (complete) {
            complete();
        }
    }); // End dispatch_async
}

@end
