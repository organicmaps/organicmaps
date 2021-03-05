protocol SubscriptionPresenterProtocol: AnyObject {
  var isLoadingHidden: Bool { get set }
  func configure()
  func purchase(anchor: UIView, period: SubscriptionPeriod)
  func restore(anchor: UIView)
  func trial(anchor: UIView)
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
  private var _debugTrial: Bool = false

  init(view: SubscriptionViewProtocol,
       router: SubscriptionRouterProtocol,
       interactor: SubscriptionInteractorProtocol,
       subscriptionManager: ISubscriptionManager) {
    self.view = view
    self.router = router
    self.interactor = interactor
    self.subscriptionManager = subscriptionManager
    debugTrial = subscriptionManager === InAppPurchase.allPassSubscriptionManager
  }

  private func configureTrial() {
    guard let trialSubscriptionItem = subscriptionGroup?[.year] else {
      fatalError()
    }
    view?.setModel(SubscriptionViewModel.trial(SubscriptionViewModel.TrialData(price: trialSubscriptionItem.formattedPrice)))
  }

  private func configureSubscriptions() {
    var data: [SubscriptionViewModel.SubscriptionData] = []
    for period in [SubscriptionPeriod.month, SubscriptionPeriod.year] {
      guard let subscriptionItem = subscriptionGroup?[period] else {
        fatalError()
      }
      data.append(SubscriptionViewModel.SubscriptionData(price: subscriptionItem.formattedPrice,
                                                         title: subscriptionItem.title,
                                                         period: period,
                                                         hasDiscount: subscriptionItem.hasDiscount,
                                                         discount: L("all_pass_screen_best_value")))
    }
    view?.setModel(SubscriptionViewModel.subsctiption(data))
  }
}

extension SubscriptionPresenter: SubscriptionPresenterProtocol {
  var debugTrial: Bool {
    get {
      _debugTrial
    }
    set {
      _debugTrial = newValue
    }
  }

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

      if self?.subscriptionManager.hasTrial == true {
        self?.subscriptionManager.checkTrialEligibility { result in
          switch result {
          case .eligible:
            self?.configureTrial()
          case .notEligible:
            self?.configureSubscriptions()
          case .serverError:
            MWMAlertViewController.activeAlert().presentInfoAlert(L("error_server_title"),
                                                                  text: L("error_server_message"))
            self?.onCancel()
          @unknown default:
            fatalError()
          }
        }
      } else {
        self?.configureSubscriptions()
      }
    }
  }

  func purchase(anchor: UIView, period: SubscriptionPeriod) {
    guard let subscription = subscriptionGroup?[period]?.subscription else {
      return
    }
    interactor.purchase(anchor: anchor, subscription: subscription, trial: false)
  }

  func onTermsPressed() {
    router.showTerms()
  }

  func onPrivacyPressed() {
    router.showPrivacy()
  }

  func onClose() {
    router.cancel()
  }

  func restore(anchor: UIView) {
    interactor.restore(anchor: anchor)
  }

  func trial(anchor: UIView) {
    guard let subscription = subscriptionGroup?[.year]?.subscription else {
      return
    }
    interactor.purchase(anchor: anchor, subscription: subscription, trial: true)
  }

  func onSubscribe() {
    router.subscribe()
  }

  func onCancel() {
    router.cancel()
  }
}
