class AllPassSubscriptionViewController: BaseSubscriptionViewController {
  //MARK:outlets
  @IBOutlet private var backgroundImageView: ImageViewCrossDisolve!
  @IBOutlet private var annualSubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var annualDiscountLabel: InsetsLabel!
  @IBOutlet private var monthlySubscriptionButton: BookmarksSubscriptionButton!
  @IBOutlet private var descriptionPageScrollView: UIScrollView!
  @IBOutlet private var contentView: UIView!
  @IBOutlet private var statusBarBackgroundView: UIVisualEffectView!
  @IBOutlet private var descriptionSubtitles: [UILabel]!

  //MARK: locals
  private var pageWidth: CGFloat {
    return self.descriptionPageScrollView.frame.width;
  }
  private let maxPages = 3;
  private var currentPage: Int {
    return Int(self.descriptionPageScrollView.contentOffset.x/self.pageWidth) + 1;
  }
  private var animatingTask: DispatchWorkItem?
  private let animationDelay: TimeInterval = 2
  private let animationDuration: TimeInterval = 0.75
  private let animationBackDuration: TimeInterval = 0.3
  private let statusBarBackVisibleThreshold: CGFloat = 60

  override var subscriptionManager: ISubscriptionManager? {
    get { return InAppPurchase.allPassSubscriptionManager }
  }
  
  override var preferredStatusBarStyle: UIStatusBarStyle {
    get { return .lightContent }
  }

  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    backgroundImageView.images = [
      UIImage.init(named: "AllPassSubscriptionBg1"),
      UIImage.init(named: "AllPassSubscriptionBg2"),
      UIImage.init(named: "AllPassSubscriptionBg3")
    ]
    startAnimating();

    annualSubscriptionButton.config(title: L("annual_subscription_title"),
                                    price: "...",
                                    enabled: false)
    monthlySubscriptionButton.config(title: L("montly_subscription_title"),
                                     price: "...",
                                     enabled: false)

    annualDiscountLabel.isHidden = true

    let fontSize: CGFloat = UIScreen.main.bounds.width > 320 ? 17.0 : 14.0
    let fontFamily = UIFont.systemFont(ofSize: fontSize).familyName
    let css = "<style type=\"text/css\">b{font-weight: 900;}body{font-weight: 300; font-size: \(fontSize); font-family: '-apple-system','\(fontFamily)';}</style>"
    zip(descriptionSubtitles, ["all_pass_subscription_message_subtitle",
                               "all_pass_subscription_message_subtitle_3",
                               "all_pass_subscription_message_subtitle_2"]).forEach { (title, loc) in
                                title.attributedText = NSAttributedString.string(withHtml: css + L(loc), defaultAttributes: [:])
    }

    self.configure(buttons: [
      .year: annualSubscriptionButton,
      .month: monthlySubscriptionButton],
                   discountLabels:[
                    .year: annualDiscountLabel])

    self.preferredContentSize = CGSize(width: 414, height: contentView.frame.height)

    Statistics.logEvent(kStatInappShow, withParameters: [kStatVendor: MWMPurchaseManager.allPassProductIds,
                                                         kStatPurchase: MWMPurchaseManager.allPassSubscriptionServerId(),
                                                         kStatProduct: MWMPurchaseManager.allPassProductIds()[0],
                                                         kStatFrom: source], with: .realtime)
  }

  @IBAction func onAnnualButtonTap(_ sender: UIButton) {
    purchase(sender: sender, period: .year)
  }

  @IBAction func onMonthlyButtonTap(_ sender: UIButton) {
    purchase(sender: sender, period: .month)
  }
}

//MARK: Animation
extension AllPassSubscriptionViewController {
  private func perform(withDelay: TimeInterval, execute: DispatchWorkItem?) {
    if let execute = execute {
      DispatchQueue.main.asyncAfter(deadline: .now() + withDelay, execute: execute)
    }
  }

  private func startAnimating() {
    if animatingTask != nil {
      animatingTask?.cancel();
    }
    animatingTask = DispatchWorkItem.init {[weak self, animationDelay] in
      self?.scrollToWithAnimation(page: (self?.currentPage ?? 0) + 1, completion: {
        self?.perform(withDelay: animationDelay, execute: self?.animatingTask)
      })
    }
    perform(withDelay: animationDelay, execute: animatingTask)
  }

  private func stopAnimating() {
    animatingTask?.cancel();
    animatingTask = nil
    view.layer.removeAllAnimations()
  }

  private func scrollToWithAnimation(page: Int, completion: @escaping ()->()) {
    var nextPage = page
    var duration = animationDuration
    if nextPage < 1 || nextPage > maxPages {
      nextPage = 1
      duration = animationBackDuration
    }

    let xOffset = CGFloat(nextPage - 1) * pageWidth
    UIView.animate(withDuration: duration,
                   delay: 0,
                   options: [.curveEaseInOut, .allowUserInteraction],
                   animations: { [weak self] in
                    self?.descriptionPageScrollView.contentOffset.x = xOffset
      }, completion:{ complete in
        completion()
    })
  }
}

extension AllPassSubscriptionViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    if scrollView == descriptionPageScrollView {
      let pageProgress = scrollView.contentOffset.x/self.pageWidth
      backgroundImageView.currentPage = pageProgress
    } else {
      let statusBarAlpha = min(scrollView.contentOffset.y/self.statusBarBackVisibleThreshold, 1)
      statusBarBackgroundView.alpha = statusBarAlpha
    }

  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    stopAnimating()
  }

  func scrollViewDidEndDragging(_ scrollView: UIScrollView, willDecelerate decelerate: Bool) {
    startAnimating()
  }
}

