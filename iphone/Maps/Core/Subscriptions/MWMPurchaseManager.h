#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MWMValidationResult) {
  MWMValidationResultValid,
  MWMValidationResultNotValid,
  MWMValidationResultServerError,
  MWMValidationResultAuthError
};

typedef NS_ENUM(NSUInteger, MWMTrialEligibilityResult) {
  MWMTrialEligibilityResultEligible,
  MWMTrialEligibilityResultNotEligible,
  MWMTrialEligibilityResultServerError
};

typedef void (^ValidateReceiptCallback)(NSString *serverId, MWMValidationResult validationResult, BOOL isTrial);
typedef void (^TrialEligibilityCallback)(NSString *serverId, MWMTrialEligibilityResult result);

typedef void (^StartTransactionCallback)(BOOL success, NSString *serverId);

@interface MWMPurchaseManager : NSObject

+ (NSString *)bookmarksSubscriptionServerId;
+ (NSString *)bookmarksSubscriptionVendorId;
+ (NSArray<NSString *> *)bookmakrsProductIds;
+ (NSString *)adsRemovalServerId;
+ (NSString *)adsRemovalVendorId;
+ (NSArray<NSString *> *)productIds;
+ (NSString *)allPassSubscriptionServerId;
+ (NSString *)allPassSubscriptionVendorId;
+ (NSArray<NSString *> *)allPassProductIds;
+ (NSArray<NSString *> *)legacyProductIds;
+ (NSArray<NSString *> *)bookmarkInappIds;
+ (void)setAdsDisabled:(BOOL)disabled;
+ (void)setBookmarksSubscriptionActive:(BOOL)active;
+ (void)setAllPassSubscriptionActive:(BOOL)active isTrial:(BOOL)isTrial;

- (instancetype)initWithVendorId:(NSString *)vendorId;
- (void)validateReceipt:(NSString *)serverId refreshReceipt:(BOOL)refresh callback:(ValidateReceiptCallback)callback;
- (void)checkTrialEligibility:(NSString *)serverId
               refreshReceipt:(BOOL)refresh
                     callback:(TrialEligibilityCallback)callback;
- (void)startTransaction:(NSString *)serverId callback:(StartTransactionCallback)callback;
- (void)refreshReceipt;

@end

NS_ASSUME_NONNULL_END
