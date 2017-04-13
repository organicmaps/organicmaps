//
//  MPRewardedVideoReward.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 * A constant that indicates that no currency type was specified with the reward.
 */
extern NSString *const kMPRewardedVideoRewardCurrencyTypeUnspecified;

/**
 * A constant that indicates that no currency amount was specified with the reward.
 */
extern NSInteger const kMPRewardedVideoRewardCurrencyAmountUnspecified;


/**
 * `MPRewardedVideoReward` contains all the information needed to reward the user for watching
 * a rewarded video ad. The class provides a currency amount and currency type.
 */

@interface MPRewardedVideoReward : NSObject

/**
 * The type of currency that should be rewarded to the user.
 *
 * An undefined currency type should be specified as `kMPRewardedVideoRewardCurrencyTypeUnspecified`.
 */
@property (nonatomic, readonly) NSString *currencyType;

/**
 * The amount of currency to reward to the user.
 *
 * An undefined currency amount should be specified as `kMPRewardedVideoRewardCurrencyAmountUnspecified`
 * wrapped as an NSNumber.
 */
@property (nonatomic, readonly) NSNumber *amount;

/**
 * Initializes the object with an undefined currency type (`kMPRewardedVideoRewardCurrencyTypeUnspecified`) and
 * the amount passed in.
 *
 * @param amount The amount of currency the user is receiving.
 */
- (instancetype)initWithCurrencyAmount:(NSNumber *)amount;

/**
 * Initializes the object's properties with the currencyType and amount.
 *
 * @param currencyType The type of currency the user is receiving.
 * @param amount The amount of currency the user is receiving.
 */
- (instancetype)initWithCurrencyType:(NSString *)currencyType amount:(NSNumber *)amount;

@end
