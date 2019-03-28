//
//  ShareInviteHelper.h
//  AppsFlyerLib
//
//  Created by Gil Meroz on 27/01/2017.
//
//

#import <Foundation/Foundation.h>
#import "AppsFlyerLinkGenerator.h"

@interface AppsFlyerShareInviteHelper : NSObject

NS_ASSUME_NONNULL_BEGIN

/*!
 *  The AppsFlyerShareInviteHelper class builds the invite URL according to various setter methods 
 *  which allow passing on additional information on the click. 
 *  This information is available through `onConversionDataReceived:` when the user accepts the invite and installs the app.
 *  In addition, campaign and channel parameters are visible within the AppsFlyer Dashboard.
 */
+ (void) generateInviteUrlWithLinkGenerator:(AppsFlyerLinkGenerator * (^)(AppsFlyerLinkGenerator *generator))generatorCreator
                          completionHandler:(void (^)(NSURL * _Nullable url))completionHandler;

/*! 
 *  It is recommended to generate an in-app event after the invite is sent to track the invites from the senders' perspective. 
 *  This enables you to find the users that tend most to invite friends, and the media sources that get you these users.
 */
+ (void) trackInvite:(nullable NSString *)channel parameters:(nullable NSDictionary *)parameters;

@end

NS_ASSUME_NONNULL_END
