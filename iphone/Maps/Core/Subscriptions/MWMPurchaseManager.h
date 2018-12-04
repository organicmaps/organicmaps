#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MWMValidationResult)
{
  MWMValidationResultValid,
  MWMValidationResultNotValid,
  MWMValidationResultError,
};

typedef void (^ValidateReceiptCallback)(NSString * serverId, MWMValidationResult validationResult);

typedef void (^StartTransactionCallback)(BOOL success, NSString * serverId);

@interface MWMPurchaseManager : NSObject

+ (NSString *)adsRemovalServerId;
+ (NSString *)adsRemovalVendorId;
+ (NSArray<NSString *> *)productIds;
+ (NSArray<NSString *> *)legacyProductIds;
+ (MWMPurchaseManager *)sharedManager;

- (void)validateReceipt:(NSString *)serverId
         refreshReceipt:(BOOL)refresh
               callback:(ValidateReceiptCallback)callback;
- (void)startTransaction:(NSString *)serverId callback:(StartTransactionCallback)callback;
- (void)setAdsDisabled:(BOOL)disabled;
- (void)refreshReceipt;

@end

NS_ASSUME_NONNULL_END
