class FadeTransitioning<T: DimmedModalPresentationController>: NSObject, UIViewControllerTransitioningDelegate {
  let presentedTransitioning = FadeInAnimatedTransitioning()
  let dismissedTransitioning = FadeOutAnimatedTransitioning()
  let isCancellable: Bool

  init(cancellable: Bool = true) {
    isCancellable = cancellable
    super.init()
  }

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
    return T(presentedViewController: presented, presenting: presenting, cancellable: isCancellable)
  }
}
