//
//  LinkGenerator.h
//  AppsFlyerLib
//
//  Created by Gil Meroz on 27/01/2017.
//
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/*!
 *  Payload container for the `generateInviteUrlWithLinkGenerator:completionHandler:` from `AppsFlyerShareInviteHelper`
 */
@interface AppsFlyerLinkGenerator: NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/// The channel through which the invite was sent (e.g. Facebook/Gmail/etc.). Usage: Recommended
- (void)setChannel           :(nonnull NSString *)channel;
- (void)setReferrerCustomerId:(nonnull NSString *)referrerCustomerId;
/// A campaign name. Usage: Optional
- (void)setCampaign          :(nonnull NSString *)campaign;
- (void)setReferrerUID       :(nonnull NSString *)referrerUID;
- (void)setReferrerName      :(nonnull NSString *)referrerName;
/// The URL to referrer user avatar. Usage: Optional
- (void)setReferrerImageURL  :(nonnull NSString *)referrerImageURL;
- (void)setAppleAppID        :(nonnull NSString *)appleAppID;
- (void)setDeeplinkPath      :(nonnull NSString *)deeplinkPath;
- (void)setBaseDeeplink      :(nonnull NSString *)baseDeeplink;
/// A single key value custom parameter. Usage: Optional
- (void)addParameterValue    :(nonnull NSString *)value forKey:(NSString*)key;
/// Multiple key value custom parameters. Usage: Optional
- (void)addParameters        :(nonnull NSDictionary *)parameters;

@end

NS_ASSUME_NONNULL_END
