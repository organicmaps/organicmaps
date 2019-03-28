//
//  CrossPromotionHelper.h
//  AppsFlyerLib
//
//  Created by Gil Meroz on 27/01/2017.
//
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


NS_ASSUME_NONNULL_BEGIN

@interface AppsFlyerCrossPromotionHelper : NSObject
+ (void) trackCrossPromoteImpression:(nonnull NSString*) appID
                            campaign:(nullable NSString*) campaign;

+ (void) trackAndOpenStore:(nonnull NSString*) appID
                  campaign:(nullable NSString *) campaign
                 paramters:(nullable NSDictionary*) parameters
                 openStore:(void (^)(NSURLSession *urlSession,NSURL *clickURL))openStoreBlock;
@end

NS_ASSUME_NONNULL_END
