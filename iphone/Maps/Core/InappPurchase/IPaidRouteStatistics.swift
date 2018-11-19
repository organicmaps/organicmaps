protocol IPaidRouteStatistics {
  func logPreviewShow()
  func logPay()
  func logCancel()
  func logPaymentSuccess()
  func logPaymentError(_ errorText: String)
  func logValidationSuccess()
  func logValidationError(_ code: Int)
}
