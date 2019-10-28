protocol WelcomeViewDelegate: class {
  func welcomeDidPressNext(_ viewContoller: UIViewController)
  func welcomeDidPressClose(_ viewContoller: UIViewController)
}

protocol WelcomeRouterProtocol {
  func onNext()
  func onClose()
}

class WelcomeRouter {
  private weak var viewController: UIViewController?
  private weak var delegate: WelcomeViewDelegate?

  init (viewController: UIViewController, delegate: WelcomeViewDelegate) {
    self.viewController = viewController
    self.delegate = delegate
  }
}

extension WelcomeRouter: WelcomeRouterProtocol {
  func onNext() {
    if let viewController = viewController {
      delegate?.welcomeDidPressNext(viewController)
    }
  }

  func onClose() {
    if let viewController = viewController {
      delegate?.welcomeDidPressClose(viewController)
    }
  }
}
