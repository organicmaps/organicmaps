protocol IBillingProduct {
  var productId: String { get }
  var localizedName: String { get }
  var price: NSDecimalNumber { get }
  var priceLocale: Locale { get }
}

enum PaymentStatus {
  case success
  case failed
  case userCancelled
  case deferred
}

protocol IInAppBilling {
  typealias ProductsCompletion = ([IBillingProduct]?, Error?) -> Void
  typealias PaymentCompletion = (PaymentStatus, Error?) -> Void

  func requestProducts(_ productIds: Set<String>, completion: @escaping ProductsCompletion)
  func makePayment(_ product: IBillingProduct, completion: @escaping PaymentCompletion)
  func finishTransaction()
}
