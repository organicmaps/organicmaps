protocol ElevationDetailsRouterProtocol: AnyObject {
  func close()
}

class ElevationDetailsRouter {
  private weak var viewController: UIViewController?

  init(viewController: UIViewController) {
    self.viewController = viewController
  }
}

extension ElevationDetailsRouter: ElevationDetailsRouterProtocol {
  func close() {
    viewController?.dismiss(animated: true, completion: nil)
  }
}
