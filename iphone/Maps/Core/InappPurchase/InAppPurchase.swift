@objc
final class InAppPurchase: NSObject {
  static func paidRoutePurchase(serverId: String,
                                productId: String) -> IPaidRoutePurchase {
    let validation = MWMPurchaseValidation(vendorId: BOOKMARKS_VENDOR)
    let billing = InAppBilling()
    return PaidRoutePurchase(serverId: serverId,
                             vendorId: BOOKMARKS_VENDOR,
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

  static func inAppBilling() -> IInAppBilling {
    return InAppBilling()
  }

  @objc static var adsRemovalSubscriptionManager: ISubscriptionManager = {
    return SubscriptionManager(productIds: MWMPurchaseManager.productIds(),
                               serverId: MWMPurchaseManager.adsRemovalServerId(),
                               vendorId: MWMPurchaseManager.adsRemovalVendorId())
  } ()

  @objc static var bookmarksSubscriptionManager: ISubscriptionManager = {
    return SubscriptionManager(productIds: MWMPurchaseManager.bookmakrsProductIds(),
                               serverId: MWMPurchaseManager.bookmarksSubscriptionServerId(),
                               vendorId: MWMPurchaseManager.bookmarksSubscriptionVendorId())
  } ()

  @objc static var allPassSubscriptionManager: ISubscriptionManager = {
    return SubscriptionManager(productIds: MWMPurchaseManager.allPassProductIds(),
                               serverId: MWMPurchaseManager.allPassSubscriptionServerId(),
                               vendorId: MWMPurchaseManager.allPassSubscriptionVendorId())
  } ()
}
