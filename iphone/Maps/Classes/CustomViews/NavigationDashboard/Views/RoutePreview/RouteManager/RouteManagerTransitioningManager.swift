@objc(MWMRouteManagerTransitioningManager)
final class RouteManagerTransitioningManager: NSObject, UIViewControllerTransitioningDelegate {
  override init() {
    super.init()
  }

  func presentationController(forPresented presented: UIViewController, presenting: UIViewController?, source _: UIViewController) -> UIPresentationController? {
      RouteManagerPresentationController(presentedViewController: presented,
                                         presenting: presenting)
  }

  func animationController(forPresented _: UIViewController, presenting _: UIViewController, source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return RouteManagerTransitioning(isPresentation: true)
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return RouteManagerTransitioning(isPresentation: false)
  }
}
