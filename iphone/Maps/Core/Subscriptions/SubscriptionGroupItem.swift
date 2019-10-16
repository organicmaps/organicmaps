import Foundation

protocol ISubscriptionGroupItem {
  var subscription: ISubscription { get }
  var productId: String { get }
  var period: SubscriptionPeriod { get }
  var price: NSDecimalNumber { get }
  var priceLocale: Locale? { get }
  var formattedPrice: String { get }
  var title: String { get }
  var discount: NSDecimalNumber { get }
  var formattedDisount: String { get }
  var hasDiscount: Bool  { get }
}

/// Proxy class for subscription, calculates discount and labels
class SubscriptionGroupItem: ISubscriptionGroupItem{
  private(set) var subscription: ISubscription
  private let formatter: Formatter
  
  var productId: String{
    return subscription.productId
  }
  var period: SubscriptionPeriod{
    return subscription.period
  }
  var price: NSDecimalNumber{
    return subscription.price
  }
  var priceLocale: Locale?{
    return subscription.priceLocale
  }
  lazy var formattedPrice: String = {
    formatter.string(for: price) ?? ""
  }()

  var title: String {
    switch subscription.period {
    case .year:
      return L("annual_subscription_title")
    case .month:
      return L("montly_subscription_title")
    case .week:
      return L("weekly_subscription_title")
    case .unknown:
      return "Unknown"
    }
  }

  private(set) var discount: NSDecimalNumber = 0
  lazy var formattedDisount: String = {
     return "- \(discount.rounding(accordingToBehavior: nil).intValue) %"
  }()
  var hasDiscount: Bool {
    return discount.compare(0) == .orderedDescending
  }

  init(_ subscription: ISubscription, discount: NSDecimalNumber, formatter: Formatter) {
    self.subscription = subscription
    self.formatter = formatter
    self.discount = discount
  }
}
