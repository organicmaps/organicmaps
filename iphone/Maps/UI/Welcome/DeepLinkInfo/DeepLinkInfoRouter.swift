protocol DeeplinkInfoViewControllerDelegate: AnyObject {
  func deeplinkInfoViewControllerDidFinish(_ viewController: UIViewController, deeplink: URL?)
}

protocol DeepLinkInfoRouterProtocol: class {
  func onNext()
}

class DeepLinkInfoRouter {
  private weak var viewController: UIViewController?
  private weak var delegate: DeeplinkInfoViewControllerDelegate?
  private var deeplinkURL: URL?

  init(viewController: UIViewController,
       delegate: DeeplinkInfoViewControllerDelegate,
       deeplink: URL?) {
    self.viewController = viewController
    self.delegate = delegate
    self.deeplinkURL = deeplink
  }
}

extension DeepLinkInfoRouter: DeepLinkInfoRouterProtocol {
  func onNext() {
    guard let viewController = viewController else { return }
    delegate?.deeplinkInfoViewControllerDidFinish(viewController, deeplink: deeplinkURL)
  }
}
