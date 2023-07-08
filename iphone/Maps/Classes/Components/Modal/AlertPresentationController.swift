final class AlertPresentationController: DimmedModalPresentationController {
  override var frameOfPresentedViewInContainerView: CGRect {
    let f = super.frameOfPresentedViewInContainerView
    let s = presentedViewController.view.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize)
    let r = CGRect(x: 0, y: 0, width: s.width, height: s.height)
    return r.offsetBy(dx: (f.width - r.width) / 2, dy: (f.height - r.height) / 2)
  }

  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    presentedViewController.view.layer.cornerRadius = 12
    presentedViewController.view.clipsToBounds = true
    guard let containerView = containerView, let presentedView = presentedView else { return }
    containerView.addSubview(presentedView)
    presentedView.center = containerView.center
    presentedView.frame = frameOfPresentedViewInContainerView
    presentedView.autoresizingMask = [.flexibleLeftMargin, .flexibleTopMargin, .flexibleRightMargin, .flexibleBottomMargin]
  }

  override func dismissalTransitionDidEnd(_ completed: Bool) {
    super.presentationTransitionDidEnd(completed)
    guard let presentedView = presentedView else { return }
    if completed {
      presentedView.removeFromSuperview()
    }
  }
}
