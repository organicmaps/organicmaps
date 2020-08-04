#import "MWMPurchaseManager.h"

#include <CoreApi/CoreApi.h>

#import <StoreKit/StoreKit.h>

@interface MWMPurchaseManager () <SKRequestDelegate>

@property(nonatomic) NSMutableDictionary<NSString *, NSMutableArray<ValidateReceiptCallback> *> *validationCallbacks;
@property(nonatomic) NSMutableDictionary<NSString *, NSMutableArray<TrialEligibilityCallback> *> *trialCallbacks;
@property(nonatomic) SKReceiptRefreshRequest *receiptRequest;
@property(nonatomic, copy) NSString *vendorId;
@property(nonatomic) id<IMWMPurchaseValidation> purchaseValidation;
@property(nonatomic) id<IMWMTrialEligibility> trialEligibility;

@end

@implementation MWMPurchaseManager

+ (NSString *)bookmarksSubscriptionServerId {
  return @(BOOKMARKS_SUBSCRIPTION_SIGHTS_SERVER_ID);
}

+ (NSString *)bookmarksSubscriptionVendorId {
  return @(BOOKMARKS_SUBSCRIPTION_VENDOR);
}

+ (NSArray *)bookmakrsProductIds {
  return @[@(BOOKMARKS_SUBSCRIPTION_SIGHTS_YEARLY_PRODUCT_ID), @(BOOKMARKS_SUBSCRIPTION_SIGHTS_MONTHLY_PRODUCT_ID)];
}

+ (NSString *)allPassSubscriptionServerId {
  return @(BOOKMARKS_SUBSCRIPTION_SERVER_ID);
}

+ (NSString *)allPassSubscriptionVendorId {
  return @(BOOKMARKS_SUBSCRIPTION_VENDOR);
}

+ (NSArray *)allPassProductIds {
  return @[@(BOOKMARKS_SUBSCRIPTION_YEARLY_PRODUCT_ID), @(BOOKMARKS_SUBSCRIPTION_MONTHLY_PRODUCT_ID)];
}

+ (NSString *)adsRemovalServerId {
  return @(ADS_REMOVAL_SERVER_ID);
}

+ (NSString *)adsRemovalVendorId {
  return @(ADS_REMOVAL_VENDOR);
}

+ (NSArray *)productIds {
  return @[@(ADS_REMOVAL_YEARLY_PRODUCT_ID), @(ADS_REMOVAL_MONTHLY_PRODUCT_ID), @(ADS_REMOVAL_WEEKLY_PRODUCT_ID)];
}

+ (NSArray *)legacyProductIds {
  auto pidVec = std::vector<std::string>(ADS_REMOVAL_NOT_USED_LIST);
  NSMutableArray *result = [NSMutableArray array];
  for (auto const &s : pidVec)
    [result addObject:@(s.c_str())];

  return [result copy];
}

+ (NSArray<NSString *> *)bookmarkInappIds {
  auto pidVec = std::vector<std::string>(BOOKMARK_INAPP_IDS);
  NSMutableArray *result = [NSMutableArray array];
  for (auto const &s : pidVec)
    [result addObject:@(s.c_str())];

  return [result copy];
}

- (instancetype)initWithVendorId:(NSString *)vendorId {
  self = [super init];
  if (self) {
    _vendorId = vendorId;
    _purchaseValidation = [[MWMPurchaseValidation alloc] initWithVendorId:vendorId];
    _trialEligibility = [[MWMTrialEligibility alloc] initWithVendorId:vendorId];
    _validationCallbacks = [NSMutableDictionary dictionary];
    _trialCallbacks = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)refreshReceipt {
  self.receiptRequest = [[SKReceiptRefreshRequest alloc] init];
  self.receiptRequest.delegate = self;
  [self.receiptRequest start];
}

- (void)validateReceipt:(NSString *)serverId refreshReceipt:(BOOL)refresh callback:(ValidateReceiptCallback)callback {
  NSMutableArray<ValidateReceiptCallback> *callbackArray = self.validationCallbacks[serverId];
  if (callbackArray) {
    [callbackArray addObject:[callback copy]];
  } else {
    self.validationCallbacks[serverId] = [NSMutableArray arrayWithObject:[callback copy]];
  }
  [self validateReceipt:serverId refreshReceipt:refresh];
}

- (void)validateReceipt:(NSString *)serverId refreshReceipt:(BOOL)refresh {
  __weak __typeof(self) ws = self;
  [self.purchaseValidation
    validateReceipt:serverId
           callback:^(MWMPurchaseValidationResult validationResult, BOOL isTrial) {
             __strong __typeof(self) self = ws;
             switch (validationResult) {
               case MWMPurchaseValidationResultValid:
                 [self notifyValidation:serverId result:MWMValidationResultValid isTrial:isTrial];
                 break;
               case MWMPurchaseValidationResultNotValid:
                 [self notifyValidation:serverId result:MWMValidationResultNotValid isTrial:NO];
                 break;
               case MWMPurchaseValidationResultError:
                 [self notifyValidation:serverId result:MWMValidationResultServerError isTrial:NO];
                 break;
               case MWMPurchaseValidationResultAuthError:
                 [self notifyValidation:serverId result:MWMValidationResultAuthError isTrial:NO];
                 break;
               case MWMPurchaseValidationResultNoReceipt:
                 if (refresh) {
                   [self refreshReceipt];
                 } else {
                   [self notifyValidation:serverId result:MWMValidationResultNotValid isTrial:NO];
                 }
                 break;
             }
           }];
}

- (void)checkTrialEligibility:(NSString *)serverId
               refreshReceipt:(BOOL)refresh
                     callback:(TrialEligibilityCallback)callback {
  NSMutableArray<TrialEligibilityCallback> *callbackArray = self.trialCallbacks[serverId];
  if (callbackArray) {
    [callbackArray addObject:[callback copy]];
  } else {
    self.trialCallbacks[serverId] = [NSMutableArray arrayWithObject:[callback copy]];
  }
  [self checkTrialEligibility:serverId refreshReceipt:refresh];
}

- (void)checkTrialEligibility:(NSString *)serverId refreshReceipt:(BOOL)refresh {
  __weak __typeof(self) ws = self;
  [self.trialEligibility
    checkTrialEligibility:serverId
                 callback:^(MWMCheckTrialEligibilityResult result) {
                   __strong __typeof(self) self = ws;
                   switch (result) {
                     case MWMCheckTrialEligibilityResultEligible:
                       [self notifyTrialEligibility:serverId result:MWMTrialEligibilityResultEligible];
                       break;
                     case MWMCheckTrialEligibilityResultNotEligible:
                       [self notifyTrialEligibility:serverId result:MWMTrialEligibilityResultNotEligible];
                       break;
                     case MWMCheckTrialEligibilityResultServerError:
                       [self notifyTrialEligibility:serverId result:MWMTrialEligibilityResultServerError];
                       break;
                     case MWMCheckTrialEligibilityResultNoReceipt:
                       if (refresh) {
                         [self refreshReceipt];
                       } else {
                         [self notifyTrialEligibility:serverId result:MWMTrialEligibilityResultNotEligible];
                       }
                       break;
                   }
                 }];
}

- (void)startTransaction:(NSString *)serverId callback:(StartTransactionCallback)callback {
  GetFramework().GetPurchase()->SetStartTransactionCallback(
    [callback](bool success, std::string const &serverId, std::string const &vendorId) {
      callback(success, @(serverId.c_str()));
    });
  GetFramework().GetPurchase()->StartTransaction(serverId.UTF8String, BOOKMARKS_VENDOR,
                                                 GetFramework().GetUser().GetAccessToken());
}

- (void)notifyValidation:(NSString *)serverId result:(MWMValidationResult)result isTrial:(BOOL)isTrial {
  NSMutableArray<ValidateReceiptCallback> *callbackArray = self.validationCallbacks[serverId];
  [callbackArray
    enumerateObjectsUsingBlock:^(ValidateReceiptCallback _Nonnull callback, NSUInteger idx, BOOL *_Nonnull stop) {
      callback(serverId, result, isTrial);
    }];
  [self.validationCallbacks removeObjectForKey:serverId];
}

- (void)notifyTrialEligibility:(NSString *)serverId result:(MWMTrialEligibilityResult)result {
  NSMutableArray<TrialEligibilityCallback> *callbackArray = self.trialCallbacks[serverId];
  [callbackArray
    enumerateObjectsUsingBlock:^(TrialEligibilityCallback _Nonnull callback, NSUInteger idx, BOOL *_Nonnull stop) {
      callback(serverId, result);
    }];
  [self.trialCallbacks removeObjectForKey:serverId];
}

+ (void)setAdsDisabled:(BOOL)disabled {
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::RemoveAds, disabled, false);
}

+ (void)setBookmarksSubscriptionActive:(BOOL)active {
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::BookmarksSights, active, false);
}

+ (void)setAllPassSubscriptionActive:(BOOL)active isTrial:(BOOL)isTrial {
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::BookmarksAll, active, isTrial);
}

#pragma mark - SKRequestDelegate

- (void)requestDidFinish:(SKRequest *)request {
  [self.validationCallbacks
    enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, NSMutableArray<ValidateReceiptCallback> *_Nonnull obj,
                                        BOOL *_Nonnull stop) {
      [self validateReceipt:key refreshReceipt:NO];
    }];
  [self.trialCallbacks
    enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, NSMutableArray<TrialEligibilityCallback> *_Nonnull obj,
                                        BOOL *_Nonnull stop) {
      [self checkTrialEligibility:key refreshReceipt:NO];
    }];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error {
  [self.trialCallbacks
    enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, NSMutableArray<TrialEligibilityCallback> *_Nonnull obj,
                                        BOOL *_Nonnull stop) {
      [self notifyValidation:key result:MWMValidationResultServerError isTrial:NO];
    }];
  [self.trialCallbacks
    enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, NSMutableArray<TrialEligibilityCallback> *_Nonnull obj,
                                        BOOL *_Nonnull stop) {
      [self notifyTrialEligibility:key result:MWMTrialEligibilityResultServerError];
    }];
}

@end
