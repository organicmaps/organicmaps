class CitySubscriptionViewController: MWMViewController {
  var presenter: SubscriptionPresenterProtocol!

  @IBOutlet private var annualSubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var annualDiscountLabel: InsetsLabel!
  @IBOutlet private var monthlySubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var contentView: UIView!
  @IBOutlet private var loadingView: UIView!

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask { return [.portrait] }
  override var preferredStatusBarStyle: UIStatusBarStyle { return .lightContent }
  private var transitioning = FadeTransitioning<IPadModalPresentationController>()

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

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
    preferredContentSize = CGSize(width: 414, height: contentView.frame.height)
  }

  @IBAction func onAnnualButtonTap(_ sender: UIButton) {
    presenter.purchase(anchor: sender, period: .year)
  }

  @IBAction func onMonthlyButtonTap(_ sender: UIButton) {
    presenter.purchase(anchor: sender, period: .month)
  }

  @IBAction func onClose(_ sender: UIButton) {
    presenter.onClose()
  }

  @IBAction func onTerms(_ sender: UIButton) {
    presenter.onTermsPressed()
  }

  @IBAction func onPrivacy(_ sender: UIButton) {
    presenter.onPrivacyPressed()
  }

  @IBAction func onRestore(sender: UIButton) {
    presenter.restore(anchor: sender)
  }
}

extension CitySubscriptionViewController: SubscriptionViewProtocol {
  var isLoadingHidden: Bool {
    get {
      return loadingView.isHidden
    }
    set {
      loadingView.isHidden = newValue
    }
  }

  func setModel(_ model: SubscriptionViewModel) {
    switch model {
    case .loading:
      annualSubscriptionButton.config(title: L("annual_subscription_title"),
                                      price: "...",
                                      enabled: false)
      monthlySubscriptionButton.config(title: L("montly_subscription_title"),
                                       price: "...",
                                       enabled: false)
      annualDiscountLabel.isHidden = true
    case let .subsctiption(subscriptionData):
      for data in subscriptionData {
        if data.period == .month {
          monthlySubscriptionButton.config(title: data.title,
                                           price: data.price,
                                           enabled: true)
        }
        if data.period == .year {
          annualSubscriptionButton.config(title: data.title,
                                          price: data.price,
                                          enabled: true)
          annualDiscountLabel.isHidden = !data.hasDiscount
          annualDiscountLabel.text = data.discount
        }
      }
    case .trial:
      assertionFailure()
    }
  }
}

extension CitySubscriptionViewController: UIAdaptivePresentationControllerDelegate {
  func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
    presenter.onCancel()
  }
}
