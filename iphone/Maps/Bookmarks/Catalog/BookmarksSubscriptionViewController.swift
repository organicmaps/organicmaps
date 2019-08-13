class BookmarksSubscriptionViewController: MWMViewController {
  @IBOutlet private var annualView: UIView!
  @IBOutlet private var monthlyView: UIView!
  @IBOutlet private var gradientView: GradientView!
  @IBOutlet private var scrollView: UIScrollView!
  @IBOutlet private var continueButton: UIButton!
  @IBOutlet var loadingView: UIView!

  private let annualViewController = BookmarksSubscriptionCellViewController()
  private let monthlyViewController = BookmarksSubscriptionCellViewController()
  private var priceFormatter: NumberFormatter?
  private var monthlySubscription: ISubscription?
  private var annualSubscription: ISubscription?
  private var selectedSubscription: ISubscription?

  var onSubscribe: MWMVoidBlock?
  var onCancel: MWMVoidBlock?

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    get { return [.portrait] }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    get { return UIColor.isNightMode() ? .lightContent : .default }
  }

  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    InAppPurchase.bookmarksSubscriptionManager.addListener(self)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    InAppPurchase.bookmarksSubscriptionManager.removeListener(self)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    gradientView.isHidden = UIColor.isNightMode()

    addChildViewController(annualViewController)
    annualView.addSubview(annualViewController.view)
    annualViewController.view.alignToSuperview()
    annualViewController.didMove(toParentViewController: self)

    addChildViewController(monthlyViewController)
    monthlyView.addSubview(monthlyViewController.view)
    monthlyViewController.view.alignToSuperview()
    monthlyViewController.didMove(toParentViewController: self)

    annualViewController.config(title: L("annual_subscription_title"),
                                subtitle: L("annual_subscription_message"),
                                price: "",
                                image: UIImage(named: "bookmarksSubscriptionYear")!)
    monthlyViewController.config(title: L("montly_subscription_title"),
                                subtitle: L("montly_subscription_message"),
                                price: "",
                                image: UIImage(named: "bookmarksSubscriptionMonth")!)
    annualViewController.setSelected(true, animated: false)
    continueButton.setTitle(L("current_location_unknown_continue_button").uppercased(), for: .normal)

    Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor: MWMPurchaseManager.bookmarksSubscriptionVendorId(),
                                                         kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
    InAppPurchase.bookmarksSubscriptionManager.getAvailableSubscriptions { [weak self] (subscriptions, error) in
      guard let subscriptions = subscriptions, subscriptions.count == 2 else {
        // TODO: hande error
        return
      }

      self?.monthlySubscription = subscriptions[0]
      self?.annualSubscription = subscriptions[1]
      self?.selectedSubscription = self?.annualSubscription

      let s = subscriptions[0]
      let formatter = NumberFormatter()
      formatter.locale = s.priceLocale
      formatter.numberStyle = .currency

      let monthlyPrice = subscriptions[0].price
      let annualPrice = subscriptions[1].price
      let discount = monthlyPrice.multiplying(by: 12).subtracting(annualPrice)
      let discountString = formatter.string(from: discount)

      self?.monthlyViewController.config(title: L("montly_subscription_title"),
                                         subtitle: L("montly_subscription_message"),
                                         price: formatter.string(from: monthlyPrice) ?? "",
                                         image: UIImage(named: "bookmarksSubscriptionMonth")!)
      self?.annualViewController.config(title: L("annual_subscription_title"),
                                        subtitle: L("annual_subscription_message"),
                                        price: formatter.string(from: annualPrice) ?? "",
                                        image: UIImage(named: "bookmarksSubscriptionYear")!,
                                        discount: (discountString != nil) ? "- \(discountString!)" : nil)
    }
  }

  @IBAction func onAnnualViewTap(_ sender: UITapGestureRecognizer) {
    guard !annualViewController.isSelected else {
      return
    }
    selectedSubscription = annualSubscription
    annualViewController.setSelected(true, animated: true)
    monthlyViewController.setSelected(false, animated: true)
    scrollView.scrollRectToVisible(annualView.convert(annualView.bounds, to: scrollView), animated: true)
    Statistics.logEvent(kStatInappSelect, withParameters: [kStatProduct: selectedSubscription!.productId,
                                                           kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
  }

  @IBAction func onMonthlyViewTap(_ sender: UITapGestureRecognizer) {
    guard !monthlyViewController.isSelected else {
      return
    }
    selectedSubscription = monthlySubscription
    annualViewController.setSelected(false, animated: true)
    monthlyViewController.setSelected(true, animated: true)
    scrollView.scrollRectToVisible(monthlyView.convert(monthlyView.bounds, to: scrollView), animated: true)
    Statistics.logEvent(kStatInappSelect, withParameters: [kStatProduct: selectedSubscription!.productId,
                                                           kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
  }

  @IBAction func onContinue(_ sender: UIButton) {
    loadingView.isHidden = false
    MWMBookmarksManager.shared().ping { [weak self] (success) in
      guard success else {
        self?.loadingView.isHidden = true
        let errorDialog = BookmarksSubscriptionFailViewController { [weak self] in
          self?.dismiss(animated: true)
        }
        self?.present(errorDialog, animated: true)
        return
      }

      guard let subscription = self?.selectedSubscription else {
        return
      }
      
      InAppPurchase.bookmarksSubscriptionManager.subscribe(to: subscription)
    }
    Statistics.logEvent(kStatInappPay, withParameters: [kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
  }

  @IBAction func onClose(_ sender: UIButton) {
    onCancel?()
    Statistics.logEvent(kStatInappCancel, withParameters: [kStatPurchase: MWMPurchaseManager.bookmarksSubscriptionServerId()])
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
      let successDialog = BookmarksSubscriptionSuccessViewController { [weak self] in
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

  func didSubsribe(_ subscription: ISubscription) {
  }

  func didDefer(_ subscription: ISubscription) {

  }
}
