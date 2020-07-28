#import "MWMPurchaseValidation.h"

#include <CoreApi/Framework.h>

static NSMutableDictionary<NSString *, NSMutableArray<ValidatePurchaseCallback> *> *callbacks =
  [NSMutableDictionary dictionary];

@interface MWMPurchaseValidation ()

@property(nonatomic, copy) NSString *vendorId;

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
  NSURL *receiptUrl = [NSBundle mainBundle].appStoreReceiptURL;
  NSData *receiptData = [NSData dataWithContentsOfURL:receiptUrl];
  if (!receiptData) {
    if (callback)
      callback(MWMPurchaseValidationResultNoReceipt, false);
    return;
  }

  GetFramework().GetPurchase()->SetValidationCallback([](auto validationCode, auto const &validationResponse) {
    MWMPurchaseValidationResult result;
    switch (validationCode) {
      case Purchase::ValidationCode::Verified:
        result = MWMPurchaseValidationResultValid;
        break;
      case Purchase::ValidationCode::NotVerified:
        result = MWMPurchaseValidationResultNotValid;
        break;
      case Purchase::ValidationCode::ServerError: {
        result = MWMPurchaseValidationResultError;
        break;
        case Purchase::ValidationCode::AuthError:
          result = MWMPurchaseValidationResultAuthError;
          break;
      }
    }

    NSString *serverId = @(validationResponse.m_info.m_serverId.c_str());
    NSMutableArray<ValidatePurchaseCallback> *callbackArray = callbacks[serverId];
    [callbackArray
      enumerateObjectsUsingBlock:^(ValidatePurchaseCallback _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        obj(result, validationResponse.m_isTrial);
      }];

    [callbacks removeObjectForKey:serverId];
  });

  NSMutableArray<ValidatePurchaseCallback> *callbackArray = callbacks[serverId];
  if (!callbackArray) {
    callbackArray = [NSMutableArray arrayWithObject:[callback copy]];
    callbacks[serverId] = callbackArray;
    Purchase::ValidationInfo vi;
    vi.m_receiptData = [receiptData base64EncodedStringWithOptions:0].UTF8String;
    vi.m_serverId = serverId.UTF8String;
    vi.m_vendorId = self.vendorId.UTF8String;
    auto const accessToken = GetFramework().GetUser().GetAccessToken();
    GetFramework().GetPurchase()->Validate(vi, accessToken);
  } else {
    [callbackArray addObject:[callback copy]];
  }
}

@end
