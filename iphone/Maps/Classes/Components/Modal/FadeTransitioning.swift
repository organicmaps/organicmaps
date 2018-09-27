class FadeTransitioning<T: UIPresentationController>: NSObject, UIViewControllerTransitioningDelegate {
  let presentedTransitioning = FadeInAnimatedTransitioning()
  let dismissedTransitioning = FadeOutAnimatedTransitioning()
  func animationController(forPresented presented: UIViewController,
                           presenting: UIViewController,
                           source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return presentedTransitioning
  }

  func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return dismissedTransitioning
  }

  func presentationController(forPresented presented: UIViewController,
                              presenting: UIViewController?,
                              source: UIViewController) -> UIPresentationController? {
    return T(presentedViewController: presented, presenting: presenting)
  }
}
