@objc(MWMRouteManagerTransitioningManager)
final class RouteManagerTransitioningManager: NSObject, UIViewControllerTransitioningDelegate {
  private var popoverSourceView: UIView!
  private var permittedArrowDirections: UIPopoverArrowDirection!

  override init() {
    super.init()
  }

  @objc init(popoverSourceView: UIView, permittedArrowDirections: UIPopoverArrowDirection) {
    self.popoverSourceView = popoverSourceView
    self.permittedArrowDirections = permittedArrowDirections
    super.init()
  }

  func presentationController(forPresented presented: UIViewController, presenting: UIViewController?, source _: UIViewController) -> UIPresentationController? {
    return alternative(iPhone: { () -> UIPresentationController in
      RouteManageriPhonePresentationController(presentedViewController: presented,
                                               presenting: presenting)
    },
    iPad: { () -> UIPresentationController in
      let popover = RouteManageriPadPresentationController(presentedViewController: presented,
                                                           presenting: presenting)
      popover.sourceView = self.popoverSourceView
      popover.sourceRect = self.popoverSourceView.bounds
      popover.permittedArrowDirections = self.permittedArrowDirections
      return popover
    })()
  }

  func animationController(forPresented _: UIViewController, presenting _: UIViewController, source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return RouteManagerTransitioning(isPresentation: true)
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return RouteManagerTransitioning(isPresentation: false)
  }
}
