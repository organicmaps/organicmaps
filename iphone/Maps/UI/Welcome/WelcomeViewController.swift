protocol WelcomeConfig {
  var image: UIImage { get }
  var title: String { get }
  var text: String { get }
  var buttonTitle: String { get }
}

protocol WelcomeViewControllerDelegate: class {
  func welcomeViewControllerDidPressNext(_ viewContoller: WelcomeViewController)
  func welcomeViewControllerDidPressClose(_ viewContoller: WelcomeViewController)
  func viewSize() -> CGSize
}

class WelcomeViewController: MWMViewController {

  weak var delegate: WelcomeViewControllerDelegate?
  
  @IBOutlet weak var image: UIImageView!
  @IBOutlet weak var alertTitle: UILabel!
  @IBOutlet weak var alertText: UILabel!
  @IBOutlet weak var nextPageButton: UIButton!
  @IBOutlet weak var containerWidth: NSLayoutConstraint!
  @IBOutlet weak var containerHeight: NSLayoutConstraint!
  
  @IBOutlet weak var imageMinHeight: NSLayoutConstraint!
  @IBOutlet weak var imageHeight: NSLayoutConstraint!
  
  @IBOutlet weak var titleTopOffset: NSLayoutConstraint!
  @IBOutlet weak var titleImageOffset: NSLayoutConstraint!
  
  var pageConfig: WelcomeConfig?
  
  class var key: String { return "" }
  
  static var shouldShowWelcome: Bool {
    get {
      return !UserDefaults.standard.bool(forKey: WhatsNewController.key)
    }
    set {
      UserDefaults.standard.set(!newValue, forKey: WhatsNewController.key)
    }
  }
  
  static func controllers(firstSession: Bool) -> [WelcomeViewController]? {
    let result = firstSession ? FirstLaunchController.controllers() : WhatsNewController.controllers()
    return result
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    configInternal()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    updateSize()
  }
  
  func updateSize() {
    let size = (delegate?.viewSize())!
    let (width, height) = (size.width, size.height)
    let hideImage = (imageHeight.multiplier * height <= imageMinHeight.constant)
    titleImageOffset.priority = hideImage ? UILayoutPriority.defaultLow : UILayoutPriority.defaultHigh
    image.isHidden = hideImage
    containerWidth.constant = width
    containerHeight.constant = height
  }

  private func configInternal() {
    if let config = pageConfig {
      image.image = config.image
      alertTitle.text = L(config.title)
      alertText.text = L(config.text)
      nextPageButton.setTitle(L(config.buttonTitle), for: .normal)
    }
  }

  @IBAction func nextPage() {
    delegate?.welcomeViewControllerDidPressNext(self)
  }
  
  @IBAction func close() {
    delegate?.welcomeViewControllerDidPressClose(self)
  }
}
