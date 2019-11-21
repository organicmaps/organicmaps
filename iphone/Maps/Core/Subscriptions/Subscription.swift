@objc enum SubscriptionPeriod: Int {
  case week
  case month
  case year
  case unknown
}

@objc protocol ISubscription {
  var productId: String { get }
  var price: NSDecimalNumber { get }
  var priceLocale: Locale? { get }
  var period: SubscriptionPeriod { get }
}

class Subscription: ISubscription {
  public static let productIds = MWMPurchaseManager.productIds() + MWMPurchaseManager.bookmakrsProductIds() + MWMPurchaseManager.allPassProductIds()
  public static let legacyProductIds = MWMPurchaseManager.legacyProductIds()
  private static let periodMap: [String: SubscriptionPeriod] = [productIds[0]: .year,
                                                                productIds[1]: .month,
                                                                productIds[2]: .week,
                                                                productIds[3]: .year,
                                                                productIds[4]: .month,
                                                                productIds[5]: .year,
                                                                productIds[6]: .month]
  var productId: String
  var period: SubscriptionPeriod
  var price: NSDecimalNumber
  var priceLocale: Locale?

  init(_ product: SKProduct) {
    productId = product.productIdentifier
    period = Subscription.periodMap[productId] ?? .unknown
    price = product.price
    priceLocale = product.priceLocale
  }
}
