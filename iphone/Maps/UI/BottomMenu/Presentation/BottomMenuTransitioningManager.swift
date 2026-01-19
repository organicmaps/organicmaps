final class BottomMenuTransitioningManager: NSObject, UIViewControllerTransitioningDelegate {
  func presentationController(forPresented presented: UIViewController, presenting: UIViewController?, source _: UIViewController) -> UIPresentationController? {
    BottomMenuPresentationController(presentedViewController: presented,
                                     presenting: presenting)
  }

  func animationController(forPresented _: UIViewController, presenting _: UIViewController, source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    BottomMenuTransitioning(isPresentation: true)
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    BottomMenuTransitioning(isPresentation: false)
  }
}
