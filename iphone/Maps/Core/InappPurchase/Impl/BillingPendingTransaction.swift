final class BillingPendingTransaction: NSObject, IBillingPendingTransaction {
  private var pendingTransaction: SKPaymentTransaction?
  
  override init() {
    super.init()
    SKPaymentQueue.default().add(self)
  }

  deinit {
    SKPaymentQueue.default().remove(self)
  }

  var status: TransactionStatus {
    let routeTransactions = SKPaymentQueue.default().transactions.filter {
      var isOk = !Subscription.legacyProductIds.contains($0.payment.productIdentifier) &&
        !Subscription.productIds.contains($0.payment.productIdentifier)
      if isOk && $0.transactionState == .purchasing {
        isOk = false
        Statistics.logEvent("Pending_purchasing_transaction",
                            withParameters: ["productId" : $0.payment.productIdentifier])
      }
      return isOk
    }

    if routeTransactions.count > 1 {
      return .error
    } else if routeTransactions.count == 1 {
      pendingTransaction = routeTransactions[0]
      switch pendingTransaction!.transactionState {
      case .purchasing:
        fallthrough
      case .failed:
        return .failed
      case .purchased:
        fallthrough
      case .restored:
        fallthrough
      case .deferred:
        return .paid
      }
    } else {
      return .none
    }
  }

  func finishTransaction() {
    guard let transaction = pendingTransaction else {
      assert(false, "There is no pending transactions")
      return
    }

    SKPaymentQueue.default().finishTransaction(transaction)
    pendingTransaction = nil
  }
}

extension BillingPendingTransaction: SKPaymentTransactionObserver {
  func paymentQueue(_ queue: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {
    // Do nothing. Only for SKPaymentQueue.default().transactions to work
  }
}
