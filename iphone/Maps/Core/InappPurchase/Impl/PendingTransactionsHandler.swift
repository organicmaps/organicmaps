final class PendingTransactionsHandler: IPendingTransactionsHandler {
  private let purchaseValidation: IMWMPurchaseValidation
  private let pendingTransaction: IBillingPendingTransaction

  init(validation: IMWMPurchaseValidation, pendingTransaction: IBillingPendingTransaction) {
    self.purchaseValidation = validation
    self.pendingTransaction = pendingTransaction
  }

  func handlePendingTransactions(_ completion: @escaping PendingTransactinsCompletion) {
    switch pendingTransaction.status {
    case .none:
      completion(.none)
      break
    case .paid:
      purchaseValidation.validateReceipt("") { [weak self] in
        switch $0 {
        case .valid:
          completion(.success)
          self?.pendingTransaction.finishTransaction()
        case .notValid:
          fallthrough
        case .error:
          completion(.error)
        case .authError:
          completion(.needAuth)
        }
      }
      break
    case .failed:
      pendingTransaction.finishTransaction()
      break
    case .error:
      completion(.error)
      break
    }
  }
}
