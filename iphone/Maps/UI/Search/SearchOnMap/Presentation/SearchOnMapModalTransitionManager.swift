@objc
final class SearchOnMapModalTransitionManager: NSObject, UIViewControllerTransitioningDelegate {

  weak var presentationController: SearchOnMapModalPresentationView?

  func animationController(forPresented presented: UIViewController, presenting: UIViewController, source: UIViewController) -> (any UIViewControllerAnimatedTransitioning)? {
    isIPad ? SideMenuPresentationAnimator() : nil
  }

  func animationController(forDismissed dismissed: UIViewController) -> (any UIViewControllerAnimatedTransitioning)? {
    isIPad ? SideMenuDismissalAnimator() : nil
  }

  func presentationController(forPresented presented: UIViewController,
                              presenting: UIViewController?,
                              source: UIViewController) -> UIPresentationController? {
    let presentationController = SearchOnMapModalPresentationController(presentedViewController: presented, presenting: presenting)
    self.presentationController = presentationController
    return presentationController
  }
}
