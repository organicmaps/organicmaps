@objc
class PromoAfterBookingViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  private var cityImageUrl: String
  private var ok: MWMVoidBlock
  private var cancel: MWMVoidBlock
  
  @IBOutlet weak var cityImageView: UIImageView!
  
  @objc init(cityImageUrl: String, ok: @escaping MWMVoidBlock, cancel: @escaping MWMVoidBlock) {
    self.cityImageUrl = cityImageUrl
    self.ok = ok
    self.cancel = cancel
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    setCityImage(cityImageUrl)
  }
  
  private func setCityImage(_ imageUrl: String) {
      cityImageView.image = UIColor.isNightMode()
          ? UIImage(named: "img_booking_popup_pholder_night")
          : UIImage(named: "img_booking_popup_pholder")
      if !imageUrl.isEmpty, let url = URL(string: imageUrl) {
        cityImageView.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    }
  }
  
  @IBAction func onOk() {
    ok()
  }
  
  @IBAction func onCancel() {
    cancel()
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
