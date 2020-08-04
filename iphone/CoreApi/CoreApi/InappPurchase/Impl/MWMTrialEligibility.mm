#import "MWMTrialEligibility.h"

#include <CoreApi/Framework.h>

static NSMutableDictionary<NSString *, NSMutableArray<CheckTrialEligibilityCallback> *> *callbacks =
  [NSMutableDictionary dictionary];

@interface MWMTrialEligibility ()

@property(nonatomic, copy) NSString *vendorId;

@end

@implementation MWMTrialEligibility

- (instancetype)initWithVendorId:(NSString *)vendorId {
  self = [super init];
  if (self) {
    _vendorId = vendorId;
  }

  return self;
}

- (void)checkTrialEligibility:(NSString *)serverId callback:(CheckTrialEligibilityCallback)callback {
  NSURL *receiptUrl = [NSBundle mainBundle].appStoreReceiptURL;
  NSData *receiptData = [NSData dataWithContentsOfURL:receiptUrl];
  if (!receiptData) {
    if (callback)
      callback(MWMCheckTrialEligibilityResultNoReceipt);
    return;
  }

  GetFramework().GetPurchase()->SetTrialEligibilityCallback([serverId](auto trialEligibilityCode) {
    MWMCheckTrialEligibilityResult result;
    switch (trialEligibilityCode) {
      case Purchase::TrialEligibilityCode::Eligible:
        result = MWMCheckTrialEligibilityResultEligible;
        break;
      case Purchase::TrialEligibilityCode::NotEligible:
        result = MWMCheckTrialEligibilityResultNotEligible;
        break;
      case Purchase::TrialEligibilityCode::ServerError:
        result = MWMCheckTrialEligibilityResultServerError;
        break;
    }

    NSMutableArray<CheckTrialEligibilityCallback> *callbackArray = callbacks[serverId];
    [callbackArray
      enumerateObjectsUsingBlock:^(CheckTrialEligibilityCallback _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        obj(result);
      }];

    [callbacks removeObjectForKey:serverId];
  });

  NSMutableArray<CheckTrialEligibilityCallback> *callbackArray = callbacks[serverId];
  if (!callbackArray) {
    callbackArray = [NSMutableArray arrayWithObject:[callback copy]];
    callbacks[serverId] = callbackArray;
    Purchase::ValidationInfo vi;
    vi.m_receiptData = [receiptData base64EncodedStringWithOptions:0].UTF8String;
    vi.m_serverId = serverId.UTF8String;
    vi.m_vendorId = self.vendorId.UTF8String;
    GetFramework().GetPurchase()->CheckTrialEligibility(vi);
  } else {
    [callbackArray addObject:[callback copy]];
  }
}

@end
