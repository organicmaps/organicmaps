fileprivate struct StoreProduct: IStoreProduct {
  var localizedName: String
  var formattedPrice: String

  init(_ product: IBillingProduct) {
    let formatter = NumberFormatter()
    formatter.numberStyle = .currency
    formatter.locale = product.priceLocale
    formattedPrice = formatter.string(from: product.price) ?? ""
    localizedName = product.localizedName
  }
}

final class PaidRoutePurchase: NSObject, IPaidRoutePurchase {
  private var serverId: String
  private var productId: String
  private let purchaseValidation: IMWMPurchaseValidation
  private let billing: IInAppBilling

  private var storeProductCompletion: StoreProductCompletion?
  private var storePaymentCompletion: StorePaymentCompletion?
  private var billingProduct: IBillingProduct?

  init(serverId: String,
       productId: String,
       purchaseValidation: IMWMPurchaseValidation,
       billing: IInAppBilling) {
    self.serverId = serverId
    self.productId = productId
    self.purchaseValidation = purchaseValidation
    self.billing = billing
    super.init()
  }

  func requestStoreProduct(_ completion: @escaping StoreProductCompletion) {
    billing.requestProducts([productId]) { [weak self] (products, error) in
      guard let product = products?.first else {
        completion(nil, error)
        return
      }
      self?.billingProduct = product
      completion(StoreProduct(product), nil)
    }
  }

  func makePayment(_ completion: @escaping StorePaymentCompletion) {
    guard let product = billingProduct else {
      assert(false, "You must call requestStoreProduct() first")
      return
    }

    storePaymentCompletion = completion
    MWMPurchaseManager.shared().startTransaction(serverId) { [weak self] (success, serverId) in
      if !success {
        self?.storePaymentCompletion?(.error, RoutePurchaseError.paymentError)
        self?.storePaymentCompletion = nil
        return
      }
      self?.billing.makePayment(product) { (status, error) in
        switch status {
        case .success:
          self?.purchased()
        case .failed:
          self?.failed(error)
        case .userCancelled:
          self?.userCancelled(error)
        case .deferred:
          self?.deferred()
        }
      }
    }
  }

  private func purchased() {
    purchaseValidation.validateReceipt(serverId, callback: { [weak self] result in
      switch result {
      case .valid:
        self?.billing.finishTransaction()
        self?.storePaymentCompletion?(.success, nil)
      case .notValid:
        self?.storePaymentCompletion?(.error, RoutePurchaseError.validationFailed)
      case .error:
        self?.storePaymentCompletion?(.error, RoutePurchaseError.validationError)
      case .authError:
        break  // TODO(@beloal)
      }
      self?.storePaymentCompletion = nil
    })
  }

  private func userCancelled(_ error: Error?) {
    storePaymentCompletion?(.userCancelled, error)
    billing.finishTransaction()
    storePaymentCompletion = nil
  }

  private func failed(_ error: Error?) {
    storePaymentCompletion?(.error, RoutePurchaseError.paymentError)
    billing.finishTransaction()
    storePaymentCompletion = nil
  }

  private func deferred() {
    storePaymentCompletion?(.success, nil)
    billing.finishTransaction()
    storePaymentCompletion = nil
  }
}
