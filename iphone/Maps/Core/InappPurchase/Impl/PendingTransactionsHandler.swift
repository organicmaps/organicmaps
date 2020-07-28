final class PendingTransactionsHandler: IPendingTransactionsHandler {
  private let purchaseValidation: IMWMPurchaseValidation
  private let pendingTransaction: IBillingPendingTransaction

  init(validation: IMWMPurchaseValidation, pendingTransaction: IBillingPendingTransaction) {
    purchaseValidation = validation
    self.pendingTransaction = pendingTransaction
  }

  func handlePendingTransactions(_ completion: @escaping PendingTransactinsCompletion) {
    switch pendingTransaction.status {
    case .none:
      completion(.none)
    case .paid:
      purchaseValidation.validateReceipt("") { [weak self] result, _ in
        switch result {
        case .valid, .notValid, .noReceipt:
          completion(.success)
          self?.pendingTransaction.finishTransaction()
        case .error:
          completion(.error)
        case .authError:
          completion(.needAuth)
        }
      }
    case .failed:
      pendingTransaction.finishTransaction()
    case .error:
      completion(.error)
    }
  }
}
