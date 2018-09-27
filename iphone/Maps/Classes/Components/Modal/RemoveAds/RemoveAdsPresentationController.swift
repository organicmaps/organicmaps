final class RemoveAdsPresentationController: DimmedModalPresentationController {
  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    guard let containerView = containerView, let presentedView = presentedView else { return }
    presentedView.frame = containerView.bounds
    presentedView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    containerView.addSubview(presentedView)
    presentedViewController.modalPresentationCapturesStatusBarAppearance = true
    presentedViewController.setNeedsStatusBarAppearanceUpdate()
  }

  override func dismissalTransitionDidEnd(_ completed: Bool) {
    super.presentationTransitionDidEnd(completed)
    guard let presentedView = presentedView else { return }
    if completed {
      presentedView.removeFromSuperview()
    }
  }
}
