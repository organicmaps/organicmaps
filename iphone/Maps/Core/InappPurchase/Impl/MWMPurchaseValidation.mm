#import "MWMPurchaseValidation.h"

#include "Framework.h"
#include "private.h"

@interface MWMPurchaseValidation ()

@property (nonatomic, copy) NSString *vendorId;
@property (nonatomic, copy) ValidatePurchaseCallback callback;

@end

@implementation MWMPurchaseValidation

- (instancetype)initWithVendorId:(NSString *)vendorId {
  self = [super init];
  if (self) {
    _vendorId = vendorId;
  }

  return self;
}

- (void)validateReceipt:(NSString *)serverId callback:(ValidatePurchaseCallback)callback {
  self.callback = callback;
  NSURL * receiptUrl = [NSBundle mainBundle].appStoreReceiptURL;
  NSData * receiptData = [NSData dataWithContentsOfURL:receiptUrl];

  if (!receiptData)
  {
    [self validationComplete:MWMPurchaseValidationResultNotValid];
    return;
  }

  GetFramework().GetPurchase()->SetValidationCallback([self](auto validationCode, auto const & validationInfo) {
    switch (validationCode) {
      case Purchase::ValidationCode::Verified:
        [self validationComplete:MWMPurchaseValidationResultValid];
        break;
      case Purchase::ValidationCode::NotVerified:
        [self validationComplete:MWMPurchaseValidationResultNotValid];
        break;
      case Purchase::ValidationCode::ServerError: {
        [self validationComplete:MWMPurchaseValidationResultError];
        break;
      case Purchase::ValidationCode::AuthError:
        [self validationComplete:MWMPurchaseValidationResultAuthError];
        break;
      }
    }
    
    GetFramework().GetPurchase()->SetValidationCallback(nullptr);
  });

  Purchase::ValidationInfo vi;
  vi.m_receiptData = [receiptData base64EncodedStringWithOptions:0].UTF8String;
  vi.m_serverId = serverId.UTF8String;
  vi.m_vendorId = self.vendorId.UTF8String;
  auto const accessToken = GetFramework().GetUser().GetAccessToken();
  GetFramework().GetPurchase()->Validate(vi, accessToken);
}

#pragma mark - Private

- (void)validationComplete:(MWMPurchaseValidationResult)result {
  if (self.callback)
    self.callback(result);

  self.callback = nil;
}

@end
