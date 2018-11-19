@objc
final class InAppPurchase: NSObject {
  static func paidRoutePurchase(serverId: String,
                                productId: String) -> IPaidRoutePurchase {
    let validation = MWMPurchaseValidation(vendorId: BOOKMARKS_VENDOR)
    let billing = InAppBilling()
    return PaidRoutePurchase(serverId: serverId,
                             productId: productId,
                             purchaseValidation: validation,
                             billing: billing)
  }

  static func paidRouteStatistics(serverId: String,
                                  productId: String) -> IPaidRouteStatistics {
    return PaidRouteStatistics(serverId: serverId, productId: productId, vendor: BOOKMARKS_VENDOR)
  }

  @objc
  static func pendingTransactionsHandler() -> IPendingTransactionsHandler {
    let validation = MWMPurchaseValidation(vendorId: BOOKMARKS_VENDOR)
    let pendingTransaction = BillingPendingTransaction()
    return PendingTransactionsHandler(validation: validation, pendingTransaction: pendingTransaction)
  }
}
