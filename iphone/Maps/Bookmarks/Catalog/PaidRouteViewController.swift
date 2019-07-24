import SafariServices

protocol PaidRouteViewControllerDelegate: AnyObject {
  func didCompletePurchase(_ viewController: PaidRouteViewController)
  func didCompleteSubscription(_ viewController: PaidRouteViewController)
  func didCancelPurchase(_ viewController: PaidRouteViewController)
}

class PaidRouteViewController: MWMViewController {
  @IBOutlet weak var previewImageView: UIImageView!
  @IBOutlet weak var productNameLabel: UILabel!
  @IBOutlet weak var routeTitleLabel: UILabel!
  @IBOutlet weak var routeAuthorLabel: UILabel!
  @IBOutlet weak var subscribeButton: UIButton!
  @IBOutlet weak var buyButton: UIButton!
  @IBOutlet weak var loadingIndicator: UIActivityIndicatorView!
  @IBOutlet weak var loadingView: UIView!

  weak var delegate: PaidRouteViewControllerDelegate?

  private let purchase: IPaidRoutePurchase
  private let statistics: IPaidRouteStatistics
  private let name: String
  private let author: String?
  private let imageUrl: URL?

  private var product: IStoreProduct?
  private var subscription: ISubscription?
  private let subscriptionManager: SubscriptionManager

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    get { return [.portrait] }
  }
  
  init(name: String,
       author: String?,
       imageUrl: URL?,
       purchase: IPaidRoutePurchase,
       statistics: IPaidRouteStatistics) {
    self.name = name
    self.author = author
    self.imageUrl = imageUrl
    self.purchase = purchase
    self.statistics = statistics
    self.subscriptionManager = InAppPurchase.bookmarksSubscriptionManager
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

    buyButton.layer.borderColor = UIColor.linkBlue()?.cgColor
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

      let titleFormat = L((s.period == .year) ? "buy_btn_for_subscription_ios_only_year" : "buy_btn_for_subscription_ios_only_mo")
      let title = String(coreFormat: titleFormat, arguments: [formatter.string(from: s.price) ?? ""])
      self?.subscribeButton.setTitle(title, for: .normal)
      self?.subscribeButton.isEnabled = true
      self?.subscription = s
    }

    statistics.logPreviewShow()
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
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
        let errorDialog = BookmarksSubscriptionFailViewController { [weak self] in
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
        let errorDialog = BookmarksSubscriptionFailViewController { [weak self] in
          self?.dismiss(animated: true)
        }
        self?.present(errorDialog, animated: true)
        return
      }

      self?.subscriptionManager.subscribe(to: subscription)
    }
  }

  @IBAction func onCancel(_ sender: UIButton) {
    statistics.logCancel()
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
  func didFailToSubscribe(_ subscription: ISubscription, error: Error?) {
    loadingView.isHidden = true
  }

  func didSubsribe(_ subscription: ISubscription) {
    loadingView.isHidden = true
    delegate?.didCompleteSubscription(self)
    let successDialog = BookmarksSubscriptionSuccessViewController { [weak self] in
      self?.dismiss(animated: true)
    }
    present(successDialog, animated: true)
  }

  func didFailToValidate(_ subscription: ISubscription, error: Error?) {
    loadingView.isHidden = true
  }

  func didDefer(_ subscription: ISubscription) {
    loadingView.isHidden = true
  }

  func validationError() {
    loadingView.isHidden = true
  }
}
