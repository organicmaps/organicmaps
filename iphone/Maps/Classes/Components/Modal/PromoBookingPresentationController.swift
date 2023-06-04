final class PromoBookingPresentationController: DimmedModalPresentationController {
  let sideMargin: CGFloat = 32.0
  let maxWidth: CGFloat = 310.0
  
  override var frameOfPresentedViewInContainerView: CGRect {
    let f = super.frameOfPresentedViewInContainerView
    let estimatedWidth = min(maxWidth, f.width - (sideMargin * 2.0))
    let s = presentedViewController.view.systemLayoutSizeFitting(CGSize(width: estimatedWidth, height: f.height), withHorizontalFittingPriority: .required, verticalFittingPriority: .defaultLow)
    let r = CGRect(x: (f.width - s.width) / 2, y: (f.height - s.height) / 2, width: s.width, height: s.height)
    return r
  }
  
  override func containerViewWillLayoutSubviews() {
    presentedView?.frame = frameOfPresentedViewInContainerView
  }
  
  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    presentedViewController.view.layer.cornerRadius = 8
    presentedViewController.view.clipsToBounds = true
    guard let containerView = containerView, let presentedView = presentedView else { return } 
    containerView.addSubview(presentedView)
    presentedView.frame = frameOfPresentedViewInContainerView
  }
  
  override func dismissalTransitionDidEnd(_ completed: Bool) {
    super.presentationTransitionDidEnd(completed)
    guard let presentedView = presentedView else { return }
    if completed {
      presentedView.removeFromSuperview()
    }
  }
}
