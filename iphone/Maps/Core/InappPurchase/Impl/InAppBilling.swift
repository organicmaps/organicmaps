private struct BillingProduct: IBillingProduct {
  var productId: String {
    product.productIdentifier
  }

  var localizedName: String {
    product.localizedTitle
  }

  var price: NSDecimalNumber {
    product.price
  }

  var priceLocale: Locale {
    product.priceLocale
  }

  let product: SKProduct

  init(_ product: SKProduct) {
    self.product = product
  }
}

final class InAppBilling: NSObject, IInAppBilling {
  private var productsCompletion: ProductsCompletion?
  private var paymentCompletion: PaymentCompletion?
  private var productRequest: SKProductsRequest?
  private var billingProduct: BillingProduct?
  private var pendingTransaction: SKPaymentTransaction?

  override init() {
    super.init()
    SKPaymentQueue.default().add(self)
  }

  deinit {
    productRequest?.cancel()
    productRequest?.delegate = nil
    SKPaymentQueue.default().remove(self)
  }

  func requestProducts(_ productIds: Set<String>, completion: @escaping ProductsCompletion) {
    productsCompletion = completion
    productRequest = SKProductsRequest(productIdentifiers: productIds)
    productRequest!.delegate = self
    productRequest!.start()
  }

  func makePayment(_ product: IBillingProduct, completion: @escaping PaymentCompletion) {
    guard let billingProduct = product as? BillingProduct else {
      assertionFailure("Wrong product type")
      return
    }

    paymentCompletion = completion
    self.billingProduct = billingProduct
    SKPaymentQueue.default().add(SKPayment(product: billingProduct.product))
  }

  func finishTransaction() {
    guard let transaction = pendingTransaction else {
      assertionFailure("You must call makePayment() first")
      return
    }

    SKPaymentQueue.default().finishTransaction(transaction)
    billingProduct = nil
    pendingTransaction = nil
  }
}

extension InAppBilling: SKProductsRequestDelegate {
  func productsRequest(_: SKProductsRequest, didReceive response: SKProductsResponse) {
    DispatchQueue.main.async { [weak self] in
      let products = response.products.map { BillingProduct($0) }
      self?.productsCompletion?(products, nil)
      self?.productsCompletion = nil
      self?.productRequest = nil
    }
  }

  func request(_: SKRequest, didFailWithError error: Error) {
    DispatchQueue.main.async { [weak self] in
      self?.productsCompletion?(nil, error)
      self?.productsCompletion = nil
      self?.productRequest = nil
    }
  }
}

extension InAppBilling: SKPaymentTransactionObserver {
  func paymentQueue(_: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {
    guard let productId = billingProduct?.productId else { return }
    for transaction in transactions {
      if transaction.payment.productIdentifier != productId { continue }
      pendingTransaction = transaction

      switch transaction.transactionState {
      case .purchasing:
        break
      case .purchased:
        paymentCompletion?(.success, nil)
      case .failed:
        if transaction.error?._code == SKError.paymentCancelled.rawValue {
          paymentCompletion?(.userCancelled, transaction.error)
        } else {
          paymentCompletion?(.failed, transaction.error)
        }
      case .restored:
        break
      case .deferred:
        paymentCompletion?(.deferred, nil)
      }
    }
  }
}
