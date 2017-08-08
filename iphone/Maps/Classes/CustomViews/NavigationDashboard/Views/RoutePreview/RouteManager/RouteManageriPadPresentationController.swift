final class RouteManageriPadPresentationController: UIPopoverPresentationController {
  private let chromeView = UIView()

  override func containerViewWillLayoutSubviews() {
    super.containerViewWillLayoutSubviews()
    (presentedViewController as? RouteManagerViewController)?.chromeView.frame = containerView!.bounds
  }

  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    guard let presentedViewController = presentedViewController as? RouteManagerViewController,
      let coordinator = presentedViewController.transitionCoordinator,
      let containerView = containerView else { return }

    presentedViewController.containerView = containerView
    presentedViewController.chromeView.frame = containerView.bounds
    presentedViewController.chromeView.alpha = 0

    coordinator.animate(alongsideTransition: { _ in
      presentedViewController.chromeView.alpha = 1
    }, completion: nil)
  }

  override func dismissalTransitionWillBegin() {
    super.dismissalTransitionWillBegin()
    guard let presentedViewController = presentedViewController as? RouteManagerViewController,
      let coordinator = presentedViewController.transitionCoordinator,
      let presentedView = presentedView else { return }

    coordinator.animate(alongsideTransition: { _ in
      presentedViewController.chromeView.alpha = 0
    }, completion: { _ in
      presentedView.removeFromSuperview()
    })
  }
}
