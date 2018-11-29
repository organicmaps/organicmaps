@objc
enum PendingTransactionsStatus: Int {
  case none
  case success
  case error
  case needAuth
}

@objc
protocol IPendingTransactionsHandler {
  typealias PendingTransactinsCompletion = (PendingTransactionsStatus) -> Void

  func handlePendingTransactions(_ completion: @escaping PendingTransactinsCompletion)
}
