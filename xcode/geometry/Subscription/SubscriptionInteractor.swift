protocol SubscriptionInteractorProtocol: class {
  func purchase(anchor: UIView, subscription: ISubscription)
  func restore(anchor: UIView)
}

class SubscriptionInteractor {
  weak var presenter: SubscriptionPresenterProtocol!
  
  private weak var viewController: UIViewController?
  private let subscriptionManager: ISubscriptionManager
  private let bookmarksManager: BookmarksManager
  
  init (viewController: UIViewController,
        subscriptionManager: ISubscriptionManager,
        bookmarksManager: BookmarksManager) {
    self.viewController = viewController
    self.subscriptionManager = subscriptionManager
    self.bookmarksManager = bookmarksManager
  }
  
  deinit {
    subscriptionManager.removeListener(self)
  }
}

extension SubscriptionInteractor: SubscriptionInteractorProtocol {
  func purchase(anchor: UIView, subscription: ISubscription) {
    subscriptionManager.addListener(self)
    viewController?.signup(anchor: anchor, source: .subscription) { [weak self] success in
      guard success else { return }
      self?.presenter.isLoadingHidden = false
      self?.bookmarksManager.ping { success in
        guard success else {
          self?.presenter.isLoadingHidden = true
          let errorDialog = SubscriptionFailViewController { [weak self] in
            self?.presenter.onCancel()
          }
          self?.viewController?.present(errorDialog, animated: true)
          return
        }
        self?.subscriptionManager.subscribe(to: subscription)
      }
    }
    Statistics.logEvent(kStatInappSelect, withParameters: [kStatPurchase: subscriptionManager.serverId,
                                                           kStatProduct: subscription.productId],
                        with: .realtime)
    Statistics.logEvent(kStatInappPay, withParameters: [kStatPurchase: subscriptionManager.serverId ],
                        with: .realtime)
  }
  
  func restore(anchor: UIView) {
    subscriptionManager.addListener(self)
    Statistics.logEvent(kStatInappRestore, withParameters: [kStatPurchase: subscriptionManager.serverId ])
    viewController?.signup(anchor: anchor, source: .subscription) { [weak self] success in
      guard success else { return }
      self?.presenter.isLoadingHidden = false
      self?.subscriptionManager.restore { result in
        self?.presenter.isLoadingHidden = true
        let alertText: String
        switch result {
        case .valid:
          alertText = L("restore_success_alert")
        case .notValid:
          alertText = L("restore_no_subscription_alert")
        case .serverError, .authError:
          alertText = L("restore_error_alert")
        @unknown default:
          fatalError()
        }
        MWMAlertViewController.activeAlert().presentInfoAlert(L("restore_subscription"), text: alertText)
      }
    }
  }
}

extension SubscriptionInteractor: SubscriptionManagerListener {
  func didFailToValidate() {
    presenter.isLoadingHidden = true
    MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                          text: L("purchase_error_subtitle"))
  }
  
  func didValidate(_ isValid: Bool) {
    presenter.isLoadingHidden = true
    if isValid {
      presenter.onSubscribe()
    } else {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                            text: L("purchase_error_subtitle"))
    }
  }
  
  func didFailToSubscribe(_ subscription: ISubscription, error: Error?) {
    presenter.isLoadingHidden = true
    MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                          text: L("purchase_error_subtitle"))
  }
  
  func didSubscribe(_ subscription: ISubscription) {
    subscriptionManager.setSubscriptionActive(true)
    bookmarksManager.resetInvalidCategories()
  }
  
  func didDefer(_ subscription: ISubscription) {
    
  }
}
