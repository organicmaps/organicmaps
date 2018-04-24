protocol WelcomeProtocolBase: AnyObject {
  static var key: String { get }

  var pageIndex: Int! { get set }

  var pageController: WelcomePageController! { get set }

  func updateSize()

  var image: UIImageView! { get set }
  var alertTitle: UILabel! { get set }
  var alertText: UILabel! { get set }
  var nextPageButton: UIButton! { get set }
  var containerWidth: NSLayoutConstraint! { get set }
  var containerHeight: NSLayoutConstraint! { get set }
  var imageMinHeight: NSLayoutConstraint! { get set }
  var imageHeight: NSLayoutConstraint! { get set }
  var titleTopOffset: NSLayoutConstraint! { get set }
  var titleImageOffset: NSLayoutConstraint! { get set }
}

extension WelcomeProtocolBase {
  static func controller(_ pageIndex: Int) -> UIViewController {
    let sb = UIStoryboard.instance(.welcome)
    let vc = sb.instantiateViewController(withIdentifier: toString(self))
    (vc as! Self).pageIndex = pageIndex
    return vc
  }

  func setup(image: UIImage, title: String, text: String, buttonTitle: String, buttonAction: Selector) {
    self.image.image = image
    alertTitle.text = title
    alertText.text = text
    nextPageButton.setTitle(buttonTitle, for: .normal)
    nextPageButton.addTarget(self, action: buttonAction, for: .touchUpInside)
  }

  func updateSize() {
    let size = pageController.view!.size
    let (width, height) = (size.width, size.height)
    let hideImage = (imageHeight.multiplier * height <= imageMinHeight.constant)
    titleImageOffset.priority = hideImage ? UILayoutPriority.defaultLow : UILayoutPriority.defaultHigh
    image.isHidden = hideImage
    containerWidth.constant = width
    containerHeight.constant = height
  }
}

struct WelcomeConfig {
  let image: UIImage
  let title: String
  let text: String
  let buttonTitle: String
  let buttonAction: Selector
}

protocol WelcomeProtocol: WelcomeProtocolBase {
  typealias ConfigBlock = (Self) -> Void
  static var welcomeConfigs: [WelcomeConfig] { get }
  static func configBlock(pageIndex: Int) -> ConfigBlock
  func config()
}

extension WelcomeProtocol {
  static var key: String { return welcomeConfigs.reduce("\(self)@") { "\($0)\($1.title):" } }

  static func configBlock(pageIndex: Int) -> ConfigBlock {
    let welcomeConfig = welcomeConfigs[pageIndex]
    return {
      $0.setup(image: welcomeConfig.image,
               title: L(welcomeConfig.title),
               text: L(welcomeConfig.text),
               buttonTitle: L(welcomeConfig.buttonTitle),
               buttonAction: welcomeConfig.buttonAction)
    }
  }
  static var pagesCount: Int { return welcomeConfigs.count }

  func config() { type(of: self).configBlock(pageIndex: pageIndex)(self) }
}
