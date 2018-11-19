enum StorePaymentStatus {
  case success
  case userCancelled
  case error
}

enum RoutePurchaseError: Error {
  case paymentError
  case validationFailed
  case validationError
}

protocol IStoreProduct {
  var localizedName: String { get }
  var formattedPrice: String { get }
}

protocol IPaidRoutePurchase: AnyObject {
  typealias StoreProductCompletion = (IStoreProduct?, Error?) -> Void
  typealias StorePaymentCompletion = (StorePaymentStatus, Error?) -> Void

  func requestStoreProduct(_ completion: @escaping StoreProductCompletion)
  func makePayment(_ completion: @escaping StorePaymentCompletion)
}
