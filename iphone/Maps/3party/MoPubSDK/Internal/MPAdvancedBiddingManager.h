//
//  MPAdvancedBiddingManager.h
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPAdvancedBidder.h"

/**
 * Internally manages all aspects related to advanced bidding.
 */
@interface MPAdvancedBiddingManager : NSObject
/**
 * A boolean value indicating whether advanced bidding is enabled. This boolean defaults to `YES`.
 * To disable advanced bidding, set this value to `NO`.
 */
@property (nonatomic, assign) BOOL advancedBiddingEnabled;

/**
 * A UTF-8 JSON string representation of the Advanced Bidding tokens.
 * @remark If `advancedBiddingEnabled` is set to `NO`, this will always return `nil`.
 */
@property (nonatomic, copy, readonly) NSString * _Nullable bidderTokensJson;

/**
 * Singleton instance of the manager.
 */
+ (MPAdvancedBiddingManager * _Nonnull)sharedManager;

/**
 Initializes each Advanced Bidder and retains a reference. If an Advanced Bidder is
 already initialized, nothing will be done.
 @param bidders Array of bidders
 @param complete Completion block
 */
- (void)initializeBidders:(NSArray<Class<MPAdvancedBidder>> * _Nonnull)bidders complete:(void(^_Nullable)(void))complete;

@end
