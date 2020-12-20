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
