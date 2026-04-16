class FadeTransitioning<T: DimmedModalPresentationController>: NSObject, UIViewControllerTransitioningDelegate {
  let presentedTransitioning = FadeInAnimatedTransitioning()
  let dismissedTransitioning = FadeOutAnimatedTransitioning()
  let isCancellable: Bool

  init(cancellable: Bool = true) {
    isCancellable = cancellable
    super.init()
  }

  func animationController(forPresented _: UIViewController,
                           presenting _: UIViewController,
                           source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    presentedTransitioning
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    dismissedTransitioning
  }

  func presentationController(forPresented presented: UIViewController,
                              presenting: UIViewController?,
                              source _: UIViewController) -> UIPresentationController? {
    T(presentedViewController: presented, presenting: presenting, cancellable: isCancellable)
  }
}
