@objc protocol ISubscriptionManager: AnyObject {
  typealias SuscriptionsCompletion = ([ISubscription]?, Error?) -> Void
  typealias ValidationCompletion = (MWMValidationResult, Bool) -> Void
  typealias TrialEligibilityCompletion = (MWMTrialEligibilityResult) -> Void

  var productIds: [String] { get }
  var serverId: String { get }
  var vendorId: String { get }
  var hasTrial: Bool { get }

  @objc static func canMakePayments() -> Bool
  @objc func getAvailableSubscriptions(_ completion: @escaping SuscriptionsCompletion)
  @objc func subscribe(to subscription: ISubscription)
  @objc func addListener(_ listener: SubscriptionManagerListener)
  @objc func removeListener(_ listener: SubscriptionManagerListener)
  @objc func validate(completion: ValidationCompletion?)
  @objc func checkTrialEligibility(completion: TrialEligibilityCompletion?)
  @objc func restore(_ callback: @escaping ValidationCompletion)
  @objc func setSubscriptionActive(_ value: Bool, isTrial: Bool)
}

@objc protocol SubscriptionManagerListener: AnyObject {
  func didFailToSubscribe(_ subscription: ISubscription, error: Error?)
  func didSubscribe(_ subscription: ISubscription)
  func didDefer(_ subscription: ISubscription)
  func didFailToValidate()
  func didValidate(_ isValid: Bool)
}
