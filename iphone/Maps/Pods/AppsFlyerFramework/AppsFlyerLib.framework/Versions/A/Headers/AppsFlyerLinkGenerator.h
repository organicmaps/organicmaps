//
//  LinkGenerator.h
//  AppsFlyerLib
//
//  Created by Gil Meroz on 27/01/2017.
//
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 Payload container for the `generateInviteUrlWithLinkGenerator:completionHandler:` from `AppsFlyerShareInviteHelper`
 */
@interface AppsFlyerLinkGenerator: NSObject

/// Instance initialization is not allowed. Use generated instance
/// from `-[AppsFlyerShareInviteHelper generateInviteUrlWithLinkGenerator:completionHandler]`
- (instancetype)init NS_UNAVAILABLE;
/// Instance initialization is not allowed. Use generated instance
/// from `-[AppsFlyerShareInviteHelper generateInviteUrlWithLinkGenerator:completionHandler]`
+ (instancetype)new NS_UNAVAILABLE;

/// The channel through which the invite was sent (e.g. Facebook/Gmail/etc.). Usage: Recommended
- (void)setChannel           :(nonnull NSString *)channel;
/// ReferrerCustomerId setter
- (void)setReferrerCustomerId:(nonnull NSString *)referrerCustomerId;
/// A campaign name. Usage: Optional
- (void)setCampaign          :(nonnull NSString *)campaign;
/// ReferrerUID setter
- (void)setReferrerUID       :(nonnull NSString *)referrerUID;
/// Referrer name
- (void)setReferrerName      :(nonnull NSString *)referrerName;
/// The URL to referrer user avatar. Usage: Optional
- (void)setReferrerImageURL  :(nonnull NSString *)referrerImageURL;
/// AppleAppID
- (void)setAppleAppID        :(nonnull NSString *)appleAppID;
/// Deeplink path
- (void)setDeeplinkPath      :(nonnull NSString *)deeplinkPath;
/// Base deeplink path
- (void)setBaseDeeplink      :(nonnull NSString *)baseDeeplink;
/// A single key value custom parameter. Usage: Optional
- (void)addParameterValue    :(nonnull NSString *)value forKey:(NSString*)key;
/// Multiple key value custom parameters. Usage: Optional
- (void)addParameters        :(nonnull NSDictionary *)parameters;

@end

NS_ASSUME_NONNULL_END
