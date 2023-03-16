final class IPadModalPresentationController: DimmedModalPresentationController {
override var frameOfPresentedViewInContainerView: CGRect {
    guard let containerView = containerView else { return CGRect.zero }
    let screenSize = UIScreen.main.bounds
    let contentSize = presentedViewController.preferredContentSize
    let r = alternative(iPhone: containerView.bounds,
                        iPad: CGRect(x: screenSize.width/2 - contentSize.width/2,
                                     y: screenSize.height/2 - contentSize.height/2,
                                     width: contentSize.width,
                                     height: contentSize.height))
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
