@objc
class PromoAfterBookingViewController: UIViewController {
  private let transitioning = FadeTransitioning<PromoBookingPresentationController>()
  private var cityImageUrl: String
  private var okClosure: MWMVoidBlock
  private var cancelClosure: MWMVoidBlock
  private var isOnButtonClosed: Bool = false
  
  @IBOutlet var cityImageView: UIImageView!
  @IBOutlet var descriptionLabel: UILabel! {
    didSet {
      let desc = L("popup_booking_download_guides_message")
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineSpacing = 3
      let attributedDesc = NSAttributedString(string: desc, attributes: [
        .font : UIFont.regular14(),
        .foregroundColor : UIColor.blackSecondaryText(),
        .paragraphStyle : paragraphStyle
        ])
      descriptionLabel.attributedText = attributedDesc
    }
  }
  
  @objc init(cityImageUrl: String, okClosure: @escaping MWMVoidBlock, cancelClosure: @escaping MWMVoidBlock) {
    self.cityImageUrl = cityImageUrl
    self.okClosure = okClosure
    self.cancelClosure = cancelClosure
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    setCityImage(cityImageUrl)
    let eventParams = [kStatProvider: kStatMapsmeGuides, kStatScenario: kStatBooking]
    Statistics.logEvent(kStatMapsmeInAppSuggestionShown, withParameters: eventParams)
  }
  
  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    if !isOnButtonClosed {
      let eventParams = [kStatProvider: kStatMapsmeGuides,
                         kStatScenario: kStatBooking,
                         kStatOption: kStatOffscreen]
      Statistics.logEvent(kStatMapsmeInAppSuggestionClosed, withParameters: eventParams)
    }
  }
  
  private func setCityImage(_ imageUrl: String) {
    cityImageView.image = UIColor.isNightMode()
        ? UIImage(named: "img_booking_popup_pholder_dark")
        : UIImage(named: "img_booking_popup_pholder_light")
    if !imageUrl.isEmpty, let url = URL(string: imageUrl) {
      cityImageView.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    }
  }
  
  @IBAction func onOk() {
    let eventParams = [kStatProvider: kStatMapsmeGuides, kStatScenario: kStatBooking]
    Statistics.logEvent(kStatMapsmeInAppSuggestionClicked, withParameters: eventParams)
    isOnButtonClosed = true
    okClosure()
  }
  
  @IBAction func onCancel() {
    let eventParams = [kStatProvider: kStatMapsmeGuides,
                       kStatScenario: kStatBooking,
                       kStatOption: kStatCancel]
    Statistics.logEvent(kStatMapsmeInAppSuggestionClosed, withParameters: eventParams)
    isOnButtonClosed = true
    cancelClosure()
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
