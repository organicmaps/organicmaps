protocol PaidRouteViewControllerDelegate: AnyObject {
  func didCompletePurchase(_ viewController: PaidRouteViewController)
  func didCancelPurchase(_ viewController: PaidRouteViewController)
}

class PaidRouteViewController: MWMViewController {
  @IBOutlet weak var previewImageView: UIImageView!
  @IBOutlet weak var productNameLabel: UILabel!
  @IBOutlet weak var routeTitleLabel: UILabel!
  @IBOutlet weak var routeAuthorLabel: UILabel!
  @IBOutlet weak var buyButton: UIButton!
  @IBOutlet weak var cancelButton: UIButton!
  @IBOutlet weak var loadingIndicator: UIActivityIndicatorView!
  @IBOutlet weak var loadingView: UIView!

  weak var delegate: PaidRouteViewControllerDelegate?

  private let purchase: IPaidRoutePurchase
  private let statistics: IPaidRouteStatistics
  private let name: String
  private let author: String?
  private let imageUrl: URL?

  private var product: IStoreProduct?

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
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    routeTitleLabel.text = name
    routeAuthorLabel.text = author
    if let url = imageUrl {
      previewImageView.af_setImage(withURL: url,
                                   placeholderImage: nil,
                                   filter: nil,
                                   progress: nil,
                                   progressQueue: DispatchQueue.main,
                                   imageTransition: .crossDissolve(kDefaultAnimationDuration),
                                   runImageTransitionIfCached: true,
                                   completion: nil)
    }

    purchase.requestStoreProduct { [weak self] (product, error) in
      self?.loadingIndicator.stopAnimating()
      guard let product = product else {
        MWMAlertViewController.activeAlert().presentInfoAlert(L("price_error_title"),
                                                              text: L("price_error_subtitle"))
        if let s = self { s.delegate?.didCancelPurchase(s) }
        return
      }
      self?.productNameLabel.text = product.localizedName
      self?.buyButton.setTitle(String(coreFormat: L("buy_btn"), arguments: [product.formattedPrice]),
                               for: .normal)
      self?.buyButton.isEnabled = true
    }
    statistics.logPreviewShow()
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

// MARK: - Event handlers

  @IBAction func onBuy(_ sender: UIButton) {
    statistics.logPay()
    loadingView.isHidden = false
    purchase.makePayment({ [weak self] (code, error) in
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

  @IBAction func onCancel(_ sender: UIButton) {
    statistics.logCancel()
    delegate?.didCancelPurchase(self)
  }
}
