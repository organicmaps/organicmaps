final class BottomMenuPresentationController: UIPresentationController {
  /// Owns the dimming backdrop so all presentation visuals live here, not in
  /// the presented view controller.
  private let chromeView = UIView()

  func setDimmingAlpha(_ alpha: CGFloat) {
    chromeView.alpha = alpha
  }

  override init(presentedViewController: UIViewController,
                presenting presentingViewController: UIViewController?) {
    super.init(presentedViewController: presentedViewController,
               presenting: presentingViewController)
    chromeView.setStyle(.presentationBackground)
  }

  override func containerViewWillLayoutSubviews() {
    super.containerViewWillLayoutSubviews()
    chromeView.frame = containerView?.bounds ?? .zero
    presentedView?.frame = frameOfPresentedViewInContainerView
  }

  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    guard let coordinator = presentedViewController.transitionCoordinator,
          let containerView = containerView else { return }

    containerView.addSubview(chromeView)
    containerView.addSubview(presentedView!)
    chromeView.frame = containerView.bounds
    chromeView.alpha = 0

    coordinator.animate(alongsideTransition: { [chromeView] _ in
      chromeView.alpha = 1
    }, completion: nil)
  }

  override func dismissalTransitionWillBegin() {
    super.dismissalTransitionWillBegin()
    guard let coordinator = presentedViewController.transitionCoordinator,
          let presentedView = presentedView else { return }

    coordinator.animate(alongsideTransition: { [chromeView] _ in
      chromeView.alpha = 0
    }, completion: { [chromeView] _ in
      chromeView.removeFromSuperview()
    })
  }
}
