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
+ (NSString * _Nonnull)adsRemovalVendorId;
+ (NSArray<NSString *> * _Nonnull)productIds;
+ (MWMPurchaseManager * _Nonnull)sharedManager;

- (void)validateReceipt:(NSString * _Nonnull)serverId
         refreshReceipt:(BOOL)refresh
               callback:(ValidateReceiptCallback _Nonnull)callback;
- (void)setAdsDisabled:(BOOL)disabled;
- (void)refreshReceipt;

@end
