#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, MWMValidationResult)
{
  MWMValidationResultValid,
  MWMValidationResultNotValid,
  MWMValidationResultError,
};

typedef void (^ValidateReceiptCallback)(NSString * _Nonnull serverId, MWMValidationResult validationResult);

@interface MWMPurchaseManager : NSObject

+ (NSString * _Nonnull)adsRemovalServerId;
+ (NSArray<NSString *> * _Nonnull)productIds;
+ (MWMPurchaseManager * _Nonnull)sharedManager;

- (void)validateReceipt:(NSString * _Nonnull)serverId
               callback:(ValidateReceiptCallback _Nonnull)callback;
- (void)setAdsDisabled:(BOOL)disabled;

@end
