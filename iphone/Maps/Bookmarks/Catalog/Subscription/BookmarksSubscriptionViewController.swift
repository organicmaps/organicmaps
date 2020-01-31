import SafariServices

@objc class BookmarksSubscriptionViewController: BaseSubscriptionViewController {
  //MARK: outlets
  @IBOutlet private var annualSubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var annualDiscountLabel: InsetsLabel!
  @IBOutlet private var monthlySubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var contentView: UIView!

  override var subscriptionManager: ISubscriptionManager? {
    get { return InAppPurchase.bookmarksSubscriptionManager }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    get { return UIColor.isNightMode() ? .lightContent : .default }
  }

  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    annualSubscriptionButton.config(title: L("annual_subscription_title"),
                                    price: "...",
                                    enabled: false)
    monthlySubscriptionButton.config(title: L("montly_subscription_title"),
                                     price: "...",
                                     enabled: false)
    annualDiscountLabel.isHidden = true

    self.configure(buttons: [
      .year: annualSubscriptionButton,
      .month: monthlySubscriptionButton],
                   discountLabels:[
      .year: annualDiscountLabel])

    self.preferredContentSize = CGSize(width: 414, height: contentView.frame.height)
  }

  @IBAction func onAnnualButtonTap(_ sender: UIButton) {
    purchase(sender: sender, period: .year)
  }

  @IBAction func onMonthlyButtonTap(_ sender: UIButton) {
    purchase(sender: sender, period: .month)
  }
}
