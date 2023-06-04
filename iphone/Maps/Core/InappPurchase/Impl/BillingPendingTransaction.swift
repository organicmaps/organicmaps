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
      pendingTransaction = routeTransactions.last
      routeTransactions.prefix(routeTransactions.count - 1).forEach {
        SKPaymentQueue.default().finishTransaction($0)
      }
    } else if routeTransactions.count == 1 {
      pendingTransaction = routeTransactions[0]
    } else {
      return .none
    }

    switch pendingTransaction!.transactionState {
    case .purchasing, .failed:
      return .failed
    case .purchased, .restored, .deferred:
      return .paid
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
