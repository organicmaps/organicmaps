import SafariServices

protocol PaidRouteViewControllerDelegate: AnyObject {
  func didCompletePurchase(_ viewController: PaidRouteViewController)
  func didCompleteSubscription(_ viewController: PaidRouteViewController)
  func didCancelPurchase(_ viewController: PaidRouteViewController)
}

class PaidRouteViewController: MWMViewController {
  @IBOutlet var scrollView: UIScrollView!
  @IBOutlet var previewImageView: UIImageView!
  @IBOutlet var productNameLabel: UILabel!
  @IBOutlet var routeTitleLabel: UILabel!
  @IBOutlet var routeAuthorLabel: UILabel!
  @IBOutlet var subscribeButton: UIButton!
  @IBOutlet var buyButton: UIButton!
  @IBOutlet var loadingIndicator: UIActivityIndicatorView!
  @IBOutlet var loadingView: UIView!

  weak var delegate: PaidRouteViewControllerDelegate?

  private let purchase: IPaidRoutePurchase
  private let statistics: IPaidRouteStatistics
  private let name: String
  private let author: String?
  private let imageUrl: URL?
  private var adjustScroll = true

  private var product: IStoreProduct?
  private var subscription: ISubscription?
  private let subscriptionManager: ISubscriptionManager

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    get { return [.portrait] }
  }
  
  init(name: String,
       author: String?,
       imageUrl: URL?,
       subscriptionType: SubscriptionGroupType,
       purchase: IPaidRoutePurchase,
       statistics: IPaidRouteStatistics) {
    self.name = name
    self.author = author
    self.imageUrl = imageUrl
    self.purchase = purchase
    self.statistics = statistics
    switch subscriptionType {
    case .sightseeing:
      self.subscriptionManager = InAppPurchase.bookmarksSubscriptionManager
    case .allPass:
      self.subscriptionManager = InAppPurchase.allPassSubscriptionManager
    }
    super.init(nibName: nil, bundle: nil)
    self.subscriptionManager.addListener(self)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    subscriptionManager.removeListener(self)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    buyButton.layer.borderColor = UIColor.linkBlue().cgColor
    routeTitleLabel.text = name
    routeAuthorLabel.text = author
    if let url = imageUrl {
      previewImageView.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    }

    var product: IStoreProduct?
    var subscriptions: [ISubscription]?

    let dispatchGroup = DispatchGroup()
    dispatchGroup.enter()
    purchase.requestStoreProduct { (p, error) in
      product = p
      dispatchGroup.leave()
    }

    dispatchGroup.enter()
    subscriptionManager.getAvailableSubscriptions { (s, error) in
      subscriptions = s
      dispatchGroup.leave()
    }

    dispatchGroup.notify(queue: .main) { [weak self] in
      self?.loadingIndicator.stopAnimating()
      guard let product = product, let subscriptions = subscriptions else {
        MWMAlertViewController.activeAlert().presentInfoAlert(L("price_error_title"),
                                                              text: L("price_error_subtitle"))
        if let s = self { s.delegate?.didCancelPurchase(s) }
        return
      }
      self?.productNameLabel.text = product.localizedName
      self?.buyButton.setTitle(String(coreFormat: L("buy_btn"), arguments: [product.formattedPrice]), for: .normal)
      self?.buyButton.isHidden = false

      let idx = Int(arc4random() % 2)
      let s = subscriptions[idx]

      let formatter = NumberFormatter()
      formatter.locale = s.priceLocale
      formatter.numberStyle = .currency

      let titleFormat = L((s.period == .year) ? "buy_btn_for_subscription_ios_only_year_version_2"
        : "buy_btn_for_subscription_ios_only_mo_version_2")
      let title = String(coreFormat: titleFormat, arguments: [formatter.string(from: s.price) ?? ""])
      self?.subscribeButton.setTitle(title, for: .normal)
      self?.subscribeButton.isEnabled = true
      self?.subscription = s
      Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor : self?.subscriptionManager.vendorId ?? "",
                                                           kStatProduct : s.productId,
                                                           kStatPurchase : self?.subscriptionManager.serverId ?? ""],
                          with: .realtime)
    }

    statistics.logPreviewShow()
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    adjustScroll = false
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    guard adjustScroll else {
      return
    }
    if previewImageView.height < 222 {
      let adjustment = 222 - previewImageView.height
      scrollView.contentInset = UIEdgeInsets(top: adjustment, left: 0, bottom: 0, right: 0)
      scrollView.contentOffset = CGPoint(x: 0, y: -adjustment)
    } else {
      scrollView.contentInset = UIEdgeInsets(top: 1, left: 0, bottom: 0, right: 0)
    }
  }

  private func pingServer(_ completion: @escaping (_ success: Bool) -> Void) {
    MWMBookmarksManager.shared().ping { (success) in
      completion(success)
    }
  }

// MARK: - Event handlers

  @IBAction func onBuy(_ sender: UIButton) {
    statistics.logPay()
    loadingView.isHidden = false
    pingServer { [weak self] (success) in
      guard success else {
        self?.loadingView.isHidden = true
        let errorDialog = SubscriptionFailViewController { [weak self] in
          self?.dismiss(animated: true)
        }
        self?.present(errorDialog, animated: true)
        return
      }

      self?.purchase.makePayment({ [weak self] (code, error) in
        self?.loadingView.isHidden = true
        switch(code) {
        case .success:
          self?.statistics.logPaymentSuccess()
          self?.statistics.logValidationSuccess()
          if let s = self { s.delegate?.didCompletePurchase(s) }
        case .userCancelled:
          // do nothing
          break
        case .error:
          if let err = error as? RoutePurchaseError {
            switch err {
            case .paymentError:
              self?.statistics.logPaymentError("")
            case .validationFailed:
              fallthrough
            case .validationError:
              self?.statistics.logPaymentSuccess()
              self?.statistics.logValidationError(err == .validationFailed ? 0 : 2)
            }
          }
          MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                                text: L("purchase_error_subtitle"))
        }
      })
    }
  }

  @IBAction func onSubscribe(_ sender: UIButton) {
    guard let subscription = subscription else {
      assertionFailure("Subscription can't be nil")
      return
    }

    loadingView.isHidden = false
    pingServer { [weak self] (success) in
      guard success else {
        self?.loadingView.isHidden = true
        let errorDialog = SubscriptionFailViewController { [weak self] in
          self?.dismiss(animated: true)
        }
        self?.present(errorDialog, animated: true)
        return
      }

      Statistics.logEvent(kStatInappSelect, withParameters: [kStatProduct : subscription.productId,
                                                             kStatPurchase : self?.subscriptionManager.serverId ?? ""])
      Statistics.logEvent(kStatInappPay, withParameters: [kStatPurchase : self?.subscriptionManager.serverId ?? ""],
                          with: .realtime)
      self?.subscriptionManager.subscribe(to: subscription)
    }
  }

  @IBAction func onCancel(_ sender: UIButton) {
    statistics.logCancel()
    Statistics.logEvent(kStatInappCancel, withParameters: [kStatPurchase : subscriptionManager.serverId])
    delegate?.didCancelPurchase(self)
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

extension PaidRouteViewController : SubscriptionManagerListener {
  func didFailToValidate() {
    loadingView.isHidden = true
    MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                          text: L("purchase_error_subtitle"))
  }

  func didValidate(_ isValid: Bool) {
    loadingView.isHidden = true
    if (isValid) {
      delegate?.didCompleteSubscription(self)
      let successDialog = SubscriptionSuccessViewController(.sightseeing) { [weak self] in
        self?.dismiss(animated: true)
      }
      present(successDialog, animated: true)
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
    subscriptionManager.setSubscriptionActive(true)
    MWMBookmarksManager.shared().resetInvalidCategories()
  }

  func didDefer(_ subscription: ISubscription) {
    loadingView.isHidden = true
  }
}
