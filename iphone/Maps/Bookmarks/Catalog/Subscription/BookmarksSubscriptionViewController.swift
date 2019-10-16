import SafariServices

@objc class BookmarksSubscriptionViewController: MWMViewController {
  //MARK: outlets
  @IBOutlet private var annualSubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var annualDiscountView: UIView!
  @IBOutlet private var annualDiscountLabel: UILabel!
  @IBOutlet private var monthlySubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var loadingView: UIView!
  
  //MARK: locals
  private var subscriptionGroup: ISubscriptionGroup?
  
  //MARK: dependency
  private let subscriptionManager: ISubscriptionManager = InAppPurchase.bookmarksSubscriptionManager
  private let bookmarksManager: MWMBookmarksManager = MWMBookmarksManager.shared()

  @objc var onSubscribe: MWMVoidBlock?
  @objc var onCancel: MWMVoidBlock?
  @objc var source: String = kStatWebView

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    get { return [.portrait] }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    get { return UIColor.isNightMode() ? .lightContent : .default }
  }

  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    subscriptionManager.addListener(self)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    subscriptionManager.removeListener(self)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    annualSubscriptionButton.config(title: L("annual_subscription_title"),
                                    price: "...",
                                    enabled: false)
    monthlySubscriptionButton.config(title: L("montly_subscription_title"),
                                     price: "...",
                                     enabled: false)
    
    if !UIColor.isNightMode() {
      annualDiscountView.layer.shadowRadius = 4
      annualDiscountView.layer.shadowOffset = CGSize(width: 0, height: 2)
      annualDiscountView.layer.shadowColor = UIColor.blackHintText().cgColor
      annualDiscountView.layer.shadowOpacity = 0.62
    }
    annualDiscountView.isHidden = true
    
    Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor: MWMPurchaseManager.bookmarksSubscriptionVendorId(),
                                                         kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId(),
                                                         kStatProduct: BOOKMARKS_SUBSCRIPTION_YEARLY_PRODUCT_ID,
                                                         kStatFrom: source], with: .realtime)
    subscriptionManager.getAvailableSubscriptions { [weak self] (subscriptions, error) in
      guard let subscriptions = subscriptions, subscriptions.count == 2 else {
        MWMAlertViewController.activeAlert().presentInfoAlert(L("price_error_title"),
                                                              text: L("price_error_subtitle"))
        self?.onCancel?()
        return
      }

      let group = SubscriptionGroup(subscriptions: subscriptions)
      self?.subscriptionGroup = group
      if let annual = group[.year]{
        self?.annualSubscriptionButton.config(title: annual.title,
                                              price: annual.formattedPrice,
                                              enabled: true)
        self?.annualDiscountView.isHidden = !annual.hasDiscount
        self?.annualDiscountLabel.text = annual.formattedDisount
      }
      if let mountly = group[.month]{
        self?.monthlySubscriptionButton.config(title: mountly.title,
                                               price: mountly.formattedPrice,
                                               enabled: true)
      }
    }
  }

  @IBAction func onAnnualButtonTap(_ sender: UIButton) {
    purchase(sender: sender, subscription: subscriptionGroup?[.year]?.subscription)
  }

  @IBAction func onMonthlyButtonTap(_ sender: UIButton) {
    purchase(sender: sender, subscription: subscriptionGroup?[.month]?.subscription)
  }

  private func purchase(sender: UIButton, subscription: ISubscription?) {
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

        guard let subscription = subscription else {
          return
        }

        self?.subscriptionManager.subscribe(to: subscription)
      }
    }
    Statistics.logEvent(kStatInappPay, withParameters: [kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()],
                        with: .realtime)
  }

  @IBAction func onRestore(_ sender: UIButton) {
    Statistics.logEvent(kStatInappRestore, withParameters: [kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
    signup(anchor: sender) { [weak self] (success) in
      guard success else { return }
      self?.loadingView.isHidden = false
      self?.subscriptionManager.restore { result in
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
    Statistics.logEvent(kStatInappCancel, withParameters: [kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
  }

  @IBAction func onTerms(_ sender: UIButton) {
    guard let url = URL(string: MWMAuthorizationViewModel.termsOfUseLink()) else { return }
    let safari = SFSafariViewController(url: url)
    self.present(safari, animated: true, completion: nil)
  }

  @IBAction func onPrivacy(_ sender: UIButton) {
    guard let url = URL(string: MWMAuthorizationViewModel.privacyPolicyLink()) else { return }
    let safari = SFSafariViewController(url: url)
    self.present(safari, animated: true, completion: nil)
  }
}

extension BookmarksSubscriptionViewController: SubscriptionManagerListener {
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
    MWMPurchaseManager.setBookmarksSubscriptionActive(true)
    bookmarksManager.resetInvalidCategories()
  }

  func didDefer(_ subscription: ISubscription) {

  }
}
