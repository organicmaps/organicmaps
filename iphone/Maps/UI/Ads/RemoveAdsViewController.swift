import SafariServices

@objc protocol RemoveAdsViewControllerDelegate: AnyObject {
  func didCompleteSubscribtion(_ viewController: RemoveAdsViewController)
  func didCancelSubscribtion(_ viewController: RemoveAdsViewController)
}

@objc class RemoveAdsViewController: MWMViewController {
  typealias VC = RemoveAdsViewController
  private let transitioning = FadeTransitioning<RemoveAdsPresentationController>()

  @IBOutlet weak var loadingView: UIView!
  @IBOutlet weak var loadingIndicator: UIActivityIndicatorView! {
    didSet {
      loadingIndicator.color = .blackPrimaryText()
    }
  }
  @IBOutlet weak var payButton: UIButton!
  @IBOutlet weak var monthButton: UIButton!
  @IBOutlet weak var weekButton: UIButton!
  @IBOutlet weak var whySupportButton: UIButton!
  @IBOutlet weak var saveLabel: UILabel!
  @IBOutlet weak var productsLoadingIndicator: UIActivityIndicatorView!
  @IBOutlet weak var whySupportView: UIView!
  @IBOutlet weak var optionsView: UIView! {
    didSet {
      optionsView.layer.borderColor = UIColor.blackDividers().cgColor
      optionsView.layer.borderWidth = 1
    }
  }
  @IBOutlet weak var moreOptionsButton: UIButton! {
    didSet {
      moreOptionsButton.setTitle(L("options_dropdown_title").uppercased(), for: .normal)
    }
  }
  @IBOutlet weak var moreOptionsButtonImage: UIImageView! {
    didSet {
      moreOptionsButtonImage.tintColor = UIColor.blackSecondaryText()
    }
  }
  @IBOutlet weak var titleLabel: UILabel! {
    didSet {
      titleLabel.text = L("remove_ads_title").uppercased()
    }
  }
  @IBOutlet weak var whySupportLabel: UILabel! {
    didSet {
      whySupportLabel.text = L("why_support").uppercased()
    }
  }
  @IBOutlet weak var optionsHeightConstraint: NSLayoutConstraint!
  @IBOutlet weak var whySupportConstraint: NSLayoutConstraint!

  @objc weak var delegate: RemoveAdsViewControllerDelegate?
  var subscriptions: [ISubscription]?

  private static func formatPrice(_ price: NSDecimalNumber?, locale: Locale?) -> String {
    guard let price = price else { return "" }
    guard let locale = locale else { return "\(price)" }
    let formatter = NumberFormatter()
    formatter.numberStyle = .currency
    formatter.locale = locale
    return formatter.string(from: price) ?? ""
  }

  private static func calculateDiscount(_ price: NSDecimalNumber?,
                                        weeklyPrice: NSDecimalNumber?,
                                        period: SubscriptionPeriod) -> NSDecimalNumber? {
    guard let price = price, let weeklyPrice = weeklyPrice else { return nil }

    switch period {
    case .week:
      return 0
    case .month:
      return weeklyPrice.multiplying(by: 52).subtracting(price.multiplying(by: 12))
    case .year:
      return weeklyPrice.multiplying(by: 52).subtracting(price)
    case .unknown:
      return nil
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    SubscriptionManager.shared.addListener(self)
    SubscriptionManager.shared.getAvailableSubscriptions { (subscriptions, error) in
      self.subscriptions = subscriptions
      self.productsLoadingIndicator.stopAnimating()
      guard let subscriptions = subscriptions else {
        MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"), text: L("purchase_error_subtitle"))
        self.delegate?.didCancelSubscribtion(self)
        return
      }
      self.saveLabel.isHidden = false
      self.optionsView.isHidden = false
      self.payButton.isEnabled = true
      assert(subscriptions.count == 3)
      let yearlyPrice = subscriptions[0].price
      let monthlyPrice = subscriptions[1].price
      let weeklyPrice = subscriptions[2].price
      let yearlyDiscount = VC.calculateDiscount(yearlyPrice,
                                                weeklyPrice: weeklyPrice,
                                                period: subscriptions[0].period)
      let monthlyDiscount = VC.calculateDiscount(monthlyPrice,
                                                 weeklyPrice: weeklyPrice,
                                                 period: subscriptions[1].period)
      let locale = subscriptions[0].priceLocale

      self.payButton.setTitle(String(coreFormat: L("paybtn_title"),
                                     arguments: [VC.formatPrice(yearlyPrice, locale: locale)]), for: .normal)
      self.saveLabel.text = String(coreFormat: L("paybtn_subtitle"),
                                   arguments: [VC.formatPrice(yearlyDiscount, locale: locale)])

      self.monthButton.setTitle(String(coreFormat: L("options_dropdown_item1"),
                                       arguments: [VC.formatPrice(monthlyPrice ?? 0, locale: locale),
                                                   VC.formatPrice(monthlyDiscount, locale: locale)]), for: .normal)
      self.weekButton.setTitle(String(coreFormat: L("options_dropdown_item2"),
                                      arguments: [VC.formatPrice(weeklyPrice ?? 0, locale: locale)]), for: .normal)
      Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor : MWMPurchaseManager.adsRemovalVendorId(),
                                                           kStatProduct : subscriptions[0].productId])
    }
  }

  override var prefersStatusBarHidden: Bool {
    return true
  }

  deinit {
    SubscriptionManager.shared.removeListener(self)
  }

  @IBAction func onClose(_ sender: Any) {
    Statistics.logEvent(kStatInappCancel)
    delegate?.didCancelSubscribtion(self)
  }

  @IBAction func onPay(_ sender: UIButton) {
    subscribe(subscriptions?[0])
  }

  @IBAction func onMonth(_ sender: UIButton) {
    subscribe(subscriptions?[1])
  }

  @IBAction func onWeek(_ sender: UIButton) {
    subscribe(subscriptions?[2])
  }

  @IBAction func onMoreOptions(_ sender: UIButton) {
    view.layoutIfNeeded()
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.moreOptionsButtonImage.transform = CGAffineTransform(rotationAngle: -CGFloat.pi + 0.01)
      self.optionsHeightConstraint.constant = 109
      self.view.layoutIfNeeded()
    }
  }

  @IBAction func onWhySupport(_ sender: UIButton) {
    whySupportView.isHidden = false
    whySupportView.alpha = 0
    whySupportConstraint.priority = .defaultHigh
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.whySupportView.alpha = 1
      self.whySupportButton.alpha = 0
    }
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

  private func subscribe(_ subscription: ISubscription?) {
    guard let subscription = subscription else {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                            text: L("purchase_error_subtitle"))
      self.delegate?.didCancelSubscribtion(self)
      return
    }
    Statistics.logEvent(kStatInappSelect, withParameters: [kStatProduct : subscription.productId])
    Statistics.logEvent(kStatInappPay)
    showPurchaseProgress()
    SubscriptionManager.shared.subscribe(to: subscription)
  }

  private func showPurchaseProgress() {
    loadingView.isHidden = false
    loadingView.alpha = 0
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.loadingView.alpha = 1
    }
  }

  private func hidePurchaseProgress() {
    UIView.animate(withDuration: kDefaultAnimationDuration, animations: {
      self.loadingView.alpha = 0
    }) { _ in
      self.loadingView.isHidden = true
    }
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return transitioning }
    set { }
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { return .custom }
    set { }
  }
}

extension RemoveAdsViewController: SubscriptionManagerListener {
  func validationError() {
    hidePurchaseProgress()
    delegate?.didCompleteSubscribtion(self)
  }

  func didSubsribe(_ subscription: ISubscription) {
    hidePurchaseProgress()
    delegate?.didCompleteSubscribtion(self)
  }

  func didFailToValidate(_ subscription: ISubscription, error: Error?) {
    hidePurchaseProgress()
    MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                          text: L("purchase_error_subtitle"))
  }

  func didDefer(_ subscription: ISubscription) {
    hidePurchaseProgress()
    delegate?.didCompleteSubscribtion(self)
  }

  func didFailToSubscribe(_ subscription: ISubscription, error: Error?) {
    if let error = error as NSError?, error.code != SKError.paymentCancelled.rawValue {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_convert_error_title"),
                                                            text: L("purchase_error_subtitle"))
    }
    
    hidePurchaseProgress()
  }
}
