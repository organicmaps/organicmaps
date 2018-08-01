final class AlertPresentationController: DimmedModalPresentationController {
  override var frameOfPresentedViewInContainerView: CGRect {
    let f = super.frameOfPresentedViewInContainerView
    let s = presentedViewController.view.systemLayoutSizeFitting(UILayoutFittingCompressedSize)
    let r = CGRect(x: 0, y: 0, width: s.width, height: s.height)
    return r.offsetBy(dx: (f.width - r.width) / 2, dy: (f.height - r.height) / 2)
  }

  override func presentationTransitionWillBegin() {
    super.presentationTransitionWillBegin()
    presentedViewController.view.layer.cornerRadius = 12
    presentedViewController.view.clipsToBounds = true
  }
}
