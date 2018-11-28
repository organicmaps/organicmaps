NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MWMPurchaseValidationResult) {
  MWMPurchaseValidationResultValid,
  MWMPurchaseValidationResultNotValid,
  MWMPurchaseValidationResultError,
  MWMPurchaseValidationResultAuthError,
};

typedef void (^ValidatePurchaseCallback)(MWMPurchaseValidationResult validationResult);

@protocol IMWMPurchaseValidation <NSObject>

- (void)validateReceipt:(NSString *)serverId callback:(ValidatePurchaseCallback)callback;

@end

NS_ASSUME_NONNULL_END
