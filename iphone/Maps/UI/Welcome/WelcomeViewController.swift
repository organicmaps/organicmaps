protocol WelcomeConfig {
  var image: UIImage { get }
  var title: String { get }
  var text: String { get }
  var buttonTitle: String { get }
}

protocol WelcomeViewControllerDelegate: class {
  func welcomeViewControllerDidPressNext(_ viewContoller: WelcomeViewController)
  func welcomeViewControllerDidPressClose(_ viewContoller: WelcomeViewController)
}

class WelcomeViewController: MWMViewController {

  weak var delegate: WelcomeViewControllerDelegate?
  
  @IBOutlet weak var image: UIImageView!
  @IBOutlet weak var alertTitle: UILabel!
  @IBOutlet weak var alertText: UILabel!
  @IBOutlet weak var nextPageButton: UIButton!

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
    return firstSession ? FirstLaunchController.controllers() : WhatsNewController.controllers()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    configInternal()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
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
