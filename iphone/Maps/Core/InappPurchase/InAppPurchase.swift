@objc
final class InAppPurchase: NSObject {
  static func paidRoutePurchase(serverId: String,
                                productId: String) -> IPaidRoutePurchase? {
    return nil
  }

  static func paidRouteStatistics(serverId: String,
                                  productId: String,
                                  testGroup: String,
                                  source: String) -> IPaidRouteStatistics {
    return PaidRouteStatistics(serverId: serverId,
                               productId: productId,
                               vendor: BOOKMARKS_VENDOR,
                               testGroup: testGroup,
                               source: source)
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
