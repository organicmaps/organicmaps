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

class SubscriptionManager: NSObject, ISubscriptionManager {
  private let paymentQueue = SKPaymentQueue.default()
  private var productsRequest: SKProductsRequest?
  private var subscriptionsComplection: SuscriptionsCompletion?
  private var products: [String: SKProduct]?
  private var pendingSubscription: ISubscription?
  private var listeners = NSHashTable<SubscriptionManagerListener>.weakObjects()
  private var restorationCallback: ValidationCompletion?

  let productIds: [String]
  let serverId: String
  let vendorId: String
  let hasTrial: Bool
  private var purchaseManager: MWMPurchaseManager?

  init(productIds: [String], serverId: String, vendorId: String) {
    self.productIds = productIds
    self.serverId = serverId
    self.vendorId = vendorId
    hasTrial = serverId == MWMPurchaseManager.allPassSubscriptionServerId()
    super.init()
    paymentQueue.add(self)
    purchaseManager = MWMPurchaseManager(vendorId: vendorId)
  }

  deinit {
    paymentQueue.remove(self)
  }

  @objc static func canMakePayments() -> Bool {
    return SKPaymentQueue.canMakePayments()
  }

  @objc func getAvailableSubscriptions(_ completion: @escaping SuscriptionsCompletion) {
    subscriptionsComplection = completion
    productsRequest = SKProductsRequest(productIdentifiers: Set(productIds))
    productsRequest!.delegate = self
    productsRequest!.start()
  }

  @objc func subscribe(to subscription: ISubscription) {
    pendingSubscription = subscription
    guard let product = products?[subscription.productId] else { return }
    paymentQueue.add(SKPayment(product: product))
  }

  @objc func addListener(_ listener: SubscriptionManagerListener) {
    listeners.add(listener)
  }

  @objc func removeListener(_ listener: SubscriptionManagerListener) {
    listeners.remove(listener)
  }

  @objc func validate(completion: ValidationCompletion? = nil) {
    validate(false, completion: completion)
  }

  @objc func restore(_ callback: @escaping ValidationCompletion) {
    validate(true) {
      callback($0, $1)
    }
  }

  @objc func setSubscriptionActive(_ value: Bool, isTrial: Bool) {
    switch serverId {
    case MWMPurchaseManager.allPassSubscriptionServerId():
      MWMPurchaseManager.setAllPassSubscriptionActive(value, isTrial: isTrial)
    case MWMPurchaseManager.bookmarksSubscriptionServerId():
      MWMPurchaseManager.setBookmarksSubscriptionActive(value)
    case MWMPurchaseManager.adsRemovalServerId():
      MWMPurchaseManager.setAdsDisabled(value)
    default:
      fatalError()
    }
  }

  private func validate(_ refreshReceipt: Bool, completion: ValidationCompletion? = nil) {
    purchaseManager?.validateReceipt(serverId, refreshReceipt: refreshReceipt) { [weak self] _, validationResult, isTrial in
      self?.logEvents(validationResult)
      if validationResult == .valid || validationResult == .notValid {
        self?.listeners.allObjects.forEach { $0.didValidate(validationResult == .valid) }
        self?.paymentQueue.transactions
          .filter { self?.productIds.contains($0.payment.productIdentifier) ?? false &&
            ($0.transactionState == .purchased || $0.transactionState == .restored)
          }
          .forEach { self?.paymentQueue.finishTransaction($0) }
      } else {
        self?.listeners.allObjects.forEach { $0.didFailToValidate() }
      }
      completion?(validationResult, isTrial)
    }
  }

  @objc func checkTrialEligibility(completion: TrialEligibilityCompletion?) {
    purchaseManager?.checkTrialEligibility(serverId, refreshReceipt: true, callback: { _, result in
      completion?(result)
    })
  }

  private func logEvents(_ validationResult: MWMValidationResult) {
    switch validationResult {
    case .valid:
      Statistics.logEvent(kStatInappValidationSuccess, withParameters: [kStatPurchase: serverId])
      Statistics.logEvent(kStatInappProductDelivered,
                          withParameters: [kStatVendor: vendorId, kStatPurchase: serverId], with: .realtime)
    case .notValid:
      Statistics.logEvent(kStatInappValidationError, withParameters: [kStatErrorCode: 0, kStatPurchase: serverId])
    case .serverError:
      Statistics.logEvent(kStatInappValidationError, withParameters: [kStatErrorCode: 2, kStatPurchase: serverId])
    case .authError:
      Statistics.logEvent(kStatInappValidationError, withParameters: [kStatErrorCode: 1, kStatPurchase: serverId])
    }
  }
}

extension SubscriptionManager: SKProductsRequestDelegate {
  func request(_ request: SKRequest, didFailWithError error: Error) {
    Statistics.logEvent(kStatInappPaymentError,
                        withParameters: [kStatError: error.localizedDescription, kStatPurchase: serverId])
    DispatchQueue.main.async { [weak self] in
      self?.subscriptionsComplection?(nil, error)
      self?.subscriptionsComplection = nil
      self?.productsRequest = nil
    }
  }

  func productsRequest(_ request: SKProductsRequest, didReceive response: SKProductsResponse) {
    guard response.products.count == productIds.count else {
      DispatchQueue.main.async { [weak self] in
        self?.subscriptionsComplection?(nil, NSError(domain: "mapsme.subscriptions", code: -1, userInfo: nil))
        self?.subscriptionsComplection = nil
        self?.productsRequest = nil
      }
      return
    }
    let subscriptions = response.products.map { Subscription($0) }
      .sorted { $0.period.rawValue < $1.period.rawValue }
    DispatchQueue.main.async { [weak self] in
      self?.products = Dictionary(uniqueKeysWithValues: response.products.map { ($0.productIdentifier, $0) })
      self?.subscriptionsComplection?(subscriptions, nil)
      self?.subscriptionsComplection = nil
      self?.productsRequest = nil
    }
  }
}

extension SubscriptionManager: SKPaymentTransactionObserver {
  func paymentQueue(_ queue: SKPaymentQueue, updatedTransactions transactions: [SKPaymentTransaction]) {
    let subscriptionTransactions = transactions.filter {
      productIds.contains($0.payment.productIdentifier)
    }
    subscriptionTransactions
      .filter { $0.transactionState == .failed }
      .forEach { processFailed($0, error: $0.error) }

    if subscriptionTransactions.contains(where: {
      $0.transactionState == .purchased || $0.transactionState == .restored
    }) {
      subscriptionTransactions
        .filter { $0.transactionState == .purchased }
        .forEach { processPurchased($0) }
      subscriptionTransactions
        .filter { $0.transactionState == .restored }
        .forEach { processRestored($0) }
      validate(false)
    }

    subscriptionTransactions
      .filter { $0.transactionState == .deferred }
      .forEach { processDeferred($0) }
  }

  private func processDeferred(_ transaction: SKPaymentTransaction) {
    if let ps = pendingSubscription, transaction.payment.productIdentifier == ps.productId {
      listeners.allObjects.forEach { $0.didDefer(ps) }
    }
  }

  private func processPurchased(_ transaction: SKPaymentTransaction) {
    paymentQueue.finishTransaction(transaction)
    if let ps = pendingSubscription, transaction.payment.productIdentifier == ps.productId {
      Statistics.logEvent(kStatInappPaymentSuccess, withParameters: [kStatPurchase: serverId])
      listeners.allObjects.forEach { $0.didSubscribe(ps) }
    }
  }

  private func processRestored(_ transaction: SKPaymentTransaction) {
    paymentQueue.finishTransaction(transaction)
    if let ps = pendingSubscription, transaction.payment.productIdentifier == ps.productId {
      listeners.allObjects.forEach { $0.didSubscribe(ps) }
    }
  }

  private func processFailed(_ transaction: SKPaymentTransaction, error: Error?) {
    paymentQueue.finishTransaction(transaction)
    if let ps = pendingSubscription, transaction.payment.productIdentifier == ps.productId {
      let errorText = error?.localizedDescription ?? ""
      Statistics.logEvent(kStatInappPaymentError,
                          withParameters: [kStatPurchase: serverId, kStatError: errorText])
      listeners.allObjects.forEach { $0.didFailToSubscribe(ps, error: error) }
    }
  }
}
