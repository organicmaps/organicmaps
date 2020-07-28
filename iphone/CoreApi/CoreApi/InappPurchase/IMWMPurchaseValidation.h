#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MWMPurchaseValidationResult) {
  MWMPurchaseValidationResultValid,
  MWMPurchaseValidationResultNotValid,
  MWMPurchaseValidationResultError,
  MWMPurchaseValidationResultAuthError,
  MWMPurchaseValidationResultNoReceipt
};

typedef void (^ValidatePurchaseCallback)(MWMPurchaseValidationResult validationResult, BOOL isTrial);

@protocol IMWMPurchaseValidation <NSObject>

- (void)validateReceipt:(NSString *)serverId callback:(ValidatePurchaseCallback)callback;

@end

NS_ASSUME_NONNULL_END
