class BookmarksSubscriptionViewController: MWMViewController {
  @IBOutlet private var annualView: UIView!
  @IBOutlet private var monthlyView: UIView!
  @IBOutlet private var gradientView: GradientView!
  @IBOutlet var scrollView: UIScrollView!
  
  private let annualViewController = BookmarksSubscriptionCellViewController()
  private let monthlyViewController = BookmarksSubscriptionCellViewController()

  var onSubscribe: MWMVoidBlock?
  var onCancel: MWMVoidBlock?

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    get { return [.portrait] }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    get { return UIColor.isNightMode() ? .lightContent : .default }
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
                                price: "$29.99",
                                image: UIImage(named: "bookmarksSubscriptionYear")!,
                                discount: "SAVE $38")
    monthlyViewController.config(title: L("montly_subscription_title"),
                                subtitle: L("montly_subscription_message"),
                                price: "$3.99",
                                image: UIImage(named: "bookmarksSubscriptionMonth")!)
    annualViewController.setSelected(true, animated: false)
  }

  @IBAction func onAnnualViewTap(_ sender: UITapGestureRecognizer) {
    guard !annualViewController.isSelected else {
      return
    }
    annualViewController.setSelected(true, animated: true)
    monthlyViewController.setSelected(false, animated: true)
    scrollView.scrollRectToVisible(annualView.convert(annualView.bounds, to: scrollView), animated: true)
  }

  @IBAction func onMonthlyViewTap(_ sender: UITapGestureRecognizer) {
    guard !monthlyViewController.isSelected else {
      return
    }
    annualViewController.setSelected(false, animated: true)
    monthlyViewController.setSelected(true, animated: true)
    scrollView.scrollRectToVisible(monthlyView.convert(monthlyView.bounds, to: scrollView), animated: true)
  }

  @IBAction func onContinue(_ sender: UIButton) {
    onSubscribe?()
  }

  @IBAction func onClose(_ sender: UIButton) {
    onCancel?()
  }
}
