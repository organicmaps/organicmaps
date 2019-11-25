#import "MWMPurchaseManager.h"
#import "MWMPurchaseValidation.h"

#include <CoreApi/Framework.h>

#import <StoreKit/StoreKit.h>

@interface MWMPurchaseManager() <SKRequestDelegate>

@property(nonatomic, copy) ValidateReceiptCallback callback;
@property(nonatomic) SKReceiptRefreshRequest *receiptRequest;
@property(nonatomic, copy) NSString *serverId;
@property(nonatomic, copy) NSString *vendorId;
@property(nonatomic) id<IMWMPurchaseValidation> purchaseValidation;

@end

@implementation MWMPurchaseManager

+ (NSString *)bookmarksSubscriptionServerId
{
  return @(BOOKMARKS_SUBSCRIPTION_SIGHTS_SERVER_ID);
}

+ (NSString *)bookmarksSubscriptionVendorId
{
  return @(BOOKMARKS_SUBSCRIPTION_VENDOR);
}

+ (NSArray *)bookmakrsProductIds
{
  return @[@(BOOKMARKS_SUBSCRIPTION_SIGHTS_YEARLY_PRODUCT_ID),
           @(BOOKMARKS_SUBSCRIPTION_SIGHTS_MONTHLY_PRODUCT_ID)];
}

+ (NSString *)allPassSubscriptionServerId
{
  return @(BOOKMARKS_SUBSCRIPTION_SERVER_ID);
}

+ (NSString *)allPassSubscriptionVendorId
{
  return @(BOOKMARKS_SUBSCRIPTION_VENDOR);
}

+ (NSArray *)allPassProductIds
{
  return @[@(BOOKMARKS_SUBSCRIPTION_YEARLY_PRODUCT_ID),
           @(BOOKMARKS_SUBSCRIPTION_MONTHLY_PRODUCT_ID)];
}

+ (NSString *)adsRemovalServerId
{
  return @(ADS_REMOVAL_SERVER_ID);
}

+ (NSString *)adsRemovalVendorId
{
  return @(ADS_REMOVAL_VENDOR);
}

+ (NSArray *)productIds
{
  return @[@(ADS_REMOVAL_YEARLY_PRODUCT_ID),
           @(ADS_REMOVAL_MONTHLY_PRODUCT_ID),
           @(ADS_REMOVAL_WEEKLY_PRODUCT_ID)];
}

+ (NSArray *)legacyProductIds
{
  auto pidVec = std::vector<std::string>(ADS_REMOVAL_NOT_USED_LIST);
  NSMutableArray *result = [NSMutableArray array];
  for (auto const & s : pidVec)
    [result addObject:@(s.c_str())];
  
  return [result copy];
}

+ (NSArray<NSString *> *)bookmarkInappIds
{
  auto pidVec = std::vector<std::string>(BOOKMARK_INAPP_IDS);
  NSMutableArray *result = [NSMutableArray array];
  for (auto const & s : pidVec)
    [result addObject:@(s.c_str())];

  return [result copy];
}

- (instancetype)initWithVendorId:(NSString *)vendorId {
  self = [super init];
  if (self) {
    _vendorId = vendorId;
    _purchaseValidation = [[MWMPurchaseValidation alloc] initWithVendorId:vendorId];
  }
  return self;
}

- (void)refreshReceipt
{
  self.receiptRequest = [[SKReceiptRefreshRequest alloc] init];
  self.receiptRequest.delegate = self;
  [self.receiptRequest start];
}

- (void)validateReceipt:(NSString *)serverId
         refreshReceipt:(BOOL)refresh
               callback:(ValidateReceiptCallback)callback
{
  self.callback = callback;
  self.serverId = serverId;
  [self validateReceipt:refresh];
}

- (void)validateReceipt:(BOOL)refresh {
  __weak __typeof(self) ws = self;
  [self.purchaseValidation validateReceipt:self.serverId callback:^(MWMPurchaseValidationResult validationResult) {
    __strong __typeof(self) self = ws;
    switch (validationResult) {
      case MWMPurchaseValidationResultValid:
        [self validReceipt];
        break;
      case MWMPurchaseValidationResultNotValid:
        [self invalidReceipt];
        break;
      case MWMPurchaseValidationResultError:
        [self serverError];
        break;
      case MWMPurchaseValidationResultAuthError:
        [self authError];
        break;
      case MWMPurchaseValidationResultNoReceipt:
        if (refresh) {
          [self refreshReceipt];
        } else {
          [self noReceipt];
        }
        break;
    }
  }];
}

- (void)startTransaction:(NSString *)serverId callback:(StartTransactionCallback)callback {
  GetFramework().GetPurchase()->SetStartTransactionCallback([callback](bool success,
                                                                       std::string const & serverId,
                                                                       std::string const & vendorId)
  {
    callback(success, @(serverId.c_str()));
  });
  GetFramework().GetPurchase()->StartTransaction(serverId.UTF8String,
                                                 BOOKMARKS_VENDOR,
                                                 GetFramework().GetUser().GetAccessToken());
}

- (void)validReceipt
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultValid);
}

- (void)noReceipt
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultNotValid);
}

- (void)invalidReceipt
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultNotValid);
}

- (void)serverError
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultServerError);
}

- (void)authError
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultAuthError);
}

- (void)appstoreError:(NSError *)error
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultServerError);
}

+ (void)setAdsDisabled:(BOOL)disabled
{
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::RemoveAds, disabled);
}

+ (void)setBookmarksSubscriptionActive:(BOOL)active {
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::BookmarksSights, active);
}

+ (void)setAllPassSubscriptionActive:(BOOL)active {
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::BookmarksAll, active);
}

#pragma mark - SKRequestDelegate

- (void)requestDidFinish:(SKRequest *)request
{
  [self validateReceipt:NO];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
  [self appstoreError:error];
}

@end
