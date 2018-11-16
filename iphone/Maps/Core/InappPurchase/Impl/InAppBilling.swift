fileprivate struct BillingProduct: IBillingProduct {
  var productId: String {
    return product.productIdentifier
  }

  var localizedName: String {
    return product.localizedTitle
  }

  var price: NSDecimalNumber {
    return product.price
  }

  var priceLocale: Locale {
    return product.priceLocale
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
      assert(false, "Wrong product type")
      return
    }
    
    paymentCompletion = completion
    self.billingProduct = billingProduct
    SKPaymentQueue.default().add(SKPayment(product: billingProduct.product))
  }

  func finishTransaction() {
    guard let transaction = pendingTransaction else {
      assert(false, "You must call makePayment() first")
      return
    }

    SKPaymentQueue.default().finishTransaction(transaction)
    billingProduct = nil
    pendingTransaction = nil
  }
}

extension InAppBilling: SKProductsRequestDelegate {
  func productsRequest(_ request: SKProductsRequest, didReceive response: SKProductsResponse) {
    let products = response.products.map { BillingProduct($0) }
    productsCompletion?(products, nil)
    productsCompletion = nil
    productRequest = nil
  }

  func request(_ request: SKRequest, didFailWithError error: Error) {
    productsCompletion?(nil, error)
    productsCompletion = nil
    productRequest = nil
  }
}

extension InAppBilling: SKPaymentTransactionObserver {
  func paymentQueue(_ queue: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {
    guard let productId = billingProduct?.productId else { return }
    transactions.forEach {
      if ($0.payment.productIdentifier != productId) { return }
      self.pendingTransaction = $0
      
      switch $0.transactionState {
      case .purchasing:
        break
      case .purchased:
        paymentCompletion?(.success, nil)
        break
      case .failed:
        if ($0.error?._code == SKError.paymentCancelled.rawValue) {
          paymentCompletion?(.userCancelled, $0.error)
        } else {
          paymentCompletion?(.failed, $0.error)
        }
        break
      case .restored:
        break
      case .deferred:
        paymentCompletion?(.deferred, nil)
        break
      }
    }
  }
}
