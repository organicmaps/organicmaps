final class AuthorizationiPhonePresentationController: UIPresentationController {
  override func containerViewWillLayoutSubviews() {
    super.containerViewWillLayoutSubviews()
    (presentedViewController as? AuthorizationViewController)?.chromeView.frame = containerView!.bounds
    presentedView?.frame = frameOfPresentedViewInContainerView
  }

  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    guard let presentedViewController = presentedViewController as? AuthorizationViewController,
      let coordinator = presentedViewController.transitionCoordinator,
      let containerView = containerView else { return }

    containerView.addSubview(presentedView!)
    presentedViewController.containerView = containerView
    presentedViewController.chromeView.frame = containerView.bounds
    presentedViewController.chromeView.alpha = 0

    coordinator.animate(alongsideTransition: { _ in
      presentedViewController.chromeView.alpha = 1
    }, completion: nil)
  }

  override func dismissalTransitionWillBegin() {
    super.dismissalTransitionWillBegin()
    guard let presentedViewController = presentedViewController as? AuthorizationViewController,
      let coordinator = presentedViewController.transitionCoordinator,
      let presentedView = presentedView else { return }

    coordinator.animate(alongsideTransition: { _ in
      presentedViewController.chromeView.alpha = 0
    }, completion: { _ in
      presentedView.removeFromSuperview()
    })
  }
}
