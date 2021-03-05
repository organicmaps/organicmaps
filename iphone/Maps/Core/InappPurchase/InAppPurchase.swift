@objc
final class InAppPurchase: NSObject {
  static func paidRoutePurchase(serverId: String,
                                productId: String) -> IPaidRoutePurchase? {
    return nil
  }

  @objc
  static func pendingTransactionsHandler() -> IPendingTransactionsHandler? {
    return nil
  }

  static func inAppBilling() -> IInAppBilling? {
    return nil
  }

  @objc static var adsRemovalSubscriptionManager: ISubscriptionManager? = nil

  @objc static var bookmarksSubscriptionManager: ISubscriptionManager? = nil

  @objc static var allPassSubscriptionManager: ISubscriptionManager? = nil
}
