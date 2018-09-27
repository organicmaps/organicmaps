#import "MWMPurchaseManager.h"

#include "Framework.h"
#include "Private.h"

#import <StoreKit/StoreKit.h>

@interface MWMPurchaseManager() <SKRequestDelegate>

@property (nonatomic) BOOL didRefreshReceipt;
@property (nonatomic, copy) ValidateReceiptCallback callback;
@property (nonatomic) SKReceiptRefreshRequest *receiptRequest;
@property (nonatomic, copy) NSString * serverId;

@end

@implementation MWMPurchaseManager

+ (NSString *)adsRemovalServerId
{
  return @(ADS_REMOVAL_SERVER_ID);
}

+ (NSArray *)productIds
{
  return @[@(ADS_REMOVAL_WEEKLY_PRODUCT_ID),
           @(ADS_REMOVAL_MONTHLY_PRODUCT_ID),
           @(ADS_REMOVAL_YEARLY_PRODUCT_ID)];
}

+ (MWMPurchaseManager *)sharedManager
{
  static MWMPurchaseManager *instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[MWMPurchaseManager alloc] init];
  });
  return instance;
}

- (void)refreshReceipt
{
  self.receiptRequest = [[SKReceiptRefreshRequest alloc] init];
  self.receiptRequest.delegate = self;
  [self.receiptRequest start];
  self.didRefreshReceipt = YES;
}

- (void)validateReceipt:(NSString *)serverId callback:(ValidateReceiptCallback)callback
{
  self.callback = callback;
  self.serverId = serverId;
  [self validateReceipt];
}

- (void)validateReceipt
{
  NSURL * receiptUrl = [NSBundle mainBundle].appStoreReceiptURL;
  NSData * receiptData = [NSData dataWithContentsOfURL:receiptUrl];

  if (!receiptData)
  {
    [self noReceipt];
    return;
  }

  GetFramework().GetPurchase()->SetValidationCallback([self](auto validationCode, auto const & validationInfo)
  {
    switch (validationCode)
    {
    case Purchase::ValidationCode::Verified:
      [self validReceipt];
      break;
    case Purchase::ValidationCode::NotVerified:
      [self invalidReceipt];
      break;
    case Purchase::ValidationCode::ServerError:
      [self serverError];
      break;
    }
  });
  Purchase::ValidationInfo vi;
  vi.m_receiptData = [receiptData base64EncodedStringWithOptions:0].UTF8String;
  vi.m_serverId = self.serverId.UTF8String;
  vi.m_vendorId = ADS_REMOVAL_VENDOR;
  GetFramework().GetPurchase()->Validate(vi, {} /* accessToken */);
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
    self.callback(self.serverId, MWMValidationResultError);
}

- (void)appstoreError:(NSError *)error
{
  if (self.callback)
    self.callback(self.serverId, MWMValidationResultError);
}

- (void)setAdsDisabled:(BOOL)disabled
{
  GetFramework().GetPurchase()->SetSubscriptionEnabled(SubscriptionType::RemoveAds, disabled);
}

#pragma mark - SKRequestDelegate

- (void)requestDidFinish:(SKRequest *)request
{
  [self validateReceipt];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
  [self appstoreError:error];
}

@end
