enum TransactionStatus {
  case none
  case paid
  case failed
  case error
}

protocol IBillingPendingTransaction {
  var status: TransactionStatus { get }
  func finishTransaction()
}
