protocol SubscriptionPresenterProtocol: AnyObject {
  var isLoadingHidden: Bool { get set }
  func configure()
  func purchase(anchor: UIView, period: SubscriptionPeriod)
  func restore(anchor: UIView)
  func onTermsPressed()
  func onPrivacyPressed()
  func onClose()
  func onSubscribe()
  func onCancel()
}

class SubscriptionPresenter {
  private weak var view: SubscriptionViewProtocol?
  private let router: SubscriptionRouterProtocol
  private let interactor: SubscriptionInteractorProtocol

  private var subscriptionGroup: ISubscriptionGroup?
  private let subscriptionManager: ISubscriptionManager
  private var source: String = kStatWebView

  init(view: SubscriptionViewProtocol,
       router: SubscriptionRouterProtocol,
       interactor: SubscriptionInteractorProtocol,
       subscriptionManager: ISubscriptionManager,
       source: String) {
    self.view = view
    self.router = router
    self.interactor = interactor
    self.subscriptionManager = subscriptionManager
    self.source = source
  }
}

extension SubscriptionPresenter: SubscriptionPresenterProtocol {
  var isLoadingHidden: Bool {
    get {
      return view?.isLoadingHidden ?? false
    }
    set {
      view?.isLoadingHidden = newValue
    }
  }

  func configure() {
    view?.setModel(SubscriptionViewModel.loading)
    subscriptionManager.getAvailableSubscriptions { [weak self] subscriptions, error in
      self?.view?.isLoadingHidden = true
      guard let subscriptions = subscriptions else {
        MWMAlertViewController.activeAlert().presentInfoAlert(L("price_error_title"),
                                                              text: L("price_error_subtitle"))
        self?.onCancel()
        return
      }

      let group = SubscriptionGroup(subscriptions: subscriptions)
      self?.subscriptionGroup = group
      var data: [SubscriptionViewModel.SubscriptionData] = []
      for period in [SubscriptionPeriod.month, SubscriptionPeriod.year] {
        guard let subscriptionItem = group[period] else {
          assertionFailure()
          return
        }
        data.append(SubscriptionViewModel.SubscriptionData(price: subscriptionItem.formattedPrice,
                                                           title: subscriptionItem.title,
                                                           period: period,
                                                           hasDiscount: subscriptionItem.hasDiscount,
                                                           discount: L("all_pass_screen_best_value")))
      }
      self?.view?.setModel(SubscriptionViewModel.subsctiption(data))
    }

    Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor: subscriptionManager.vendorId,
                                                         kStatPurchase: subscriptionManager.serverId,
                                                         kStatProduct: subscriptionManager.productIds[0],
                                                         kStatFrom: source], with: .realtime)
  }

  func purchase(anchor: UIView, period: SubscriptionPeriod) {
    guard let subscription = subscriptionGroup?[period]?.subscription else {
      return
    }
    interactor.purchase(anchor: anchor, subscription: subscription)
    Statistics.logEvent(kStatInappSelect, withParameters: [kStatPurchase: subscriptionManager.serverId,
                                                           kStatProduct: subscription.productId],
                        with: .realtime)
    Statistics.logEvent(kStatInappPay, withParameters: [kStatPurchase: subscriptionManager.serverId],
                        with: .realtime)
  }

  func onTermsPressed() {
    router.showTerms()
  }

  func onPrivacyPressed() {
    router.showPrivacy()
  }

  func onClose() {
    router.cancel()
    Statistics.logEvent(kStatInappCancel, withParameters: [kStatPurchase: subscriptionManager.serverId])
  }

  func restore(anchor: UIView) {
    interactor.restore(anchor: anchor)
  }

  func onSubscribe() {
    router.subscribe()
  }

  func onCancel() {
    router.cancel()
  }
}
