import SafariServices

class BaseSubscriptionViewController: MWMViewController {
  //MARK: base outlets
  @IBOutlet private var loadingView: UIView!

  //MARK: dependency
  private(set) var subscriptionManager: ISubscriptionManager?
  private let bookmarksManager: MWMBookmarksManager = MWMBookmarksManager.shared()

  private var subscriptionGroup: ISubscriptionGroup?
  @objc var onSubscribe: MWMVoidBlock?
  @objc var onCancel: MWMVoidBlock?
  @objc var source: String = kStatWebView
  private let transitioning = FadeTransitioning<IPadModalPresentationController>()

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    get { return [.portrait] }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    get { return .lightContent }
  }

  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    if UIDevice.current.userInterfaceIdiom == .pad {
      transitioningDelegate = transitioning
      modalPresentationStyle = .custom
    } else {
      modalPresentationStyle = .fullScreen
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    subscriptionManager?.removeListener(self)
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    subscriptionManager?.addListener(self)
    loadingView.isHidden = false
  }

  func configure(buttons: [SubscriptionPeriod: BookmarksSubscriptionButton],
                 discountLabels: [SubscriptionPeriod: InsetsLabel]) {
    subscriptionManager?.getAvailableSubscriptions { [weak self] (subscriptions, error) in
      self?.loadingView.isHidden = true
      guard let subscriptions = subscriptions, subscriptions.count >= buttons.count else {
        MWMAlertViewController.activeAlert().presentInfoAlert(L("price_error_title"),
                                                              text: L("price_error_subtitle"))
        self?.onCancel?()
        return
      }

      let group = SubscriptionGroup(subscriptions: subscriptions)
      self?.subscriptionGroup = group
      for (period, button) in buttons {
        if let subscriptionItem = group[period] {
          button.config(title: subscriptionItem.title,
                        price: subscriptionItem.formattedPrice,
                        enabled: true)

          if subscriptionItem.hasDiscount, let discountLabel = discountLabels[period] {
            discountLabel.isHidden = false;
            discountLabel.text = L("all_pass_screen_best_value")
          }
        }
      }
    }

    Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor: subscriptionManager?.vendorId ?? "",
                                                         kStatPurchase: subscriptionManager?.serverId ?? "",
                                                         kStatProduct: subscriptionManager?.productIds[0] ?? "",
                                                         kStatFrom: source], with: .realtime)
  }

  func purchase(sender: UIButton, period: SubscriptionPeriod) {
    guard let subscription = subscriptionGroup?[period]?.subscription else{
      return
    }
    signup(anchor: sender) { [weak self] success in
      guard success else { return }
      self?.loadingView.isHidden = false
      self?.bookmarksManager.ping { success in
        guard success else {
          self?.loadingView.isHidden = true
          let errorDialog = SubscriptionFailViewController { [weak self] in
            self?.dismiss(animated: true)
          }
          self?.present(errorDialog, animated: true)
          return
        }
        self?.subscriptionManager?.subscribe(to: subscription)
      }
    }
    Statistics.logEvent(kStatInappPay, withParameters: [kStatPurchase: subscriptionManager?.serverId ?? ""],
                        with: .realtime)
  }

  @IBAction func onRestore(_ sender: UIButton) {
    Statistics.logEvent(kStatInappRestore, withParameters: [kStatPurchase: subscriptionManager?.serverId ?? ""])
    signup(anchor: sender) { [weak self] (success) in
      guard success else { return }
      self?.loadingView.isHidden = false
      self?.subscriptionManager?.restore { result in
        self?.loadingView.isHidden = true
        let alertText: String
        switch result {
        case .valid:
          alertText = L("restore_success_alert")
        case .notValid:
          alertText = L("restore_no_subscription_alert")
        case .serverError, .authError:
          alertText = L("restore_error_alert")
        }
        MWMAlertViewController.activeAlert().presentInfoAlert(L("restore_subscription"), text: alertText)
      }
    }
  }

  @IBAction func onClose(_ sender: UIButton) {
    onCancel?()
    Statistics.logEvent(kStatInappCancel, withParameters: [kStatPurchase: subscriptionManager?.serverId ?? ""])
  }

  @IBAction func onTerms(_ sender: UIButton) {
    guard let url = URL(string: User.termsOfUseLink()) else { return }
    let safari = SFSafariViewController(url: url)
    self.present(safari, animated: true, completion: nil)
  }

  @IBAction func onPrivacy(_ sender: UIButton) {
    guard let url = URL(string: User.privacyPolicyLink()) else { return }
    let safari = SFSafariViewController(url: url)
    self.present(safari, animated: true, completion: nil)
  }
}

extension BaseSubscriptionViewController: UIAdaptivePresentationControllerDelegate {
  func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
    onCancel?()
  }
}

extension BaseSubscriptionViewController: SubscriptionManagerListener {
  func didFailToValidate() {
    loadingView.isHidden = true
    MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                          text: L("purchase_error_subtitle"))
  }

  func didValidate(_ isValid: Bool) {
    loadingView.isHidden = true
    if (isValid) {
      onSubscribe?()
    } else {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                            text: L("purchase_error_subtitle"))
    }
  }

  func didFailToSubscribe(_ subscription: ISubscription, error: Error?) {
    loadingView.isHidden = true
    MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                          text: L("purchase_error_subtitle"))
  }

  func didSubscribe(_ subscription: ISubscription) {
    subscriptionManager?.setSubscriptionActive(true)
    bookmarksManager.resetInvalidCategories()
  }

  func didDefer(_ subscription: ISubscription) {

  }
}


