final class CoverVerticalModalTransitioning: NSObject, UIViewControllerTransitioningDelegate {
  private var height: CGFloat
  init(presentationHeight: CGFloat) {
    height = presentationHeight
  }

  func animationController(forPresented presented: UIViewController,
                           presenting: UIViewController,
                           source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return CoverVerticalPresentationAnimator()
  }

  func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return CoverVerticalDismissalAnimator()
  }

  func presentationController(forPresented presented: UIViewController,
                              presenting: UIViewController?,
                              source: UIViewController) -> UIPresentationController? {
    return PresentationController(presentedViewController: presented, presenting: presenting, presentationHeight: height)
  }
}

fileprivate final class PresentationController: DimmedModalPresentationController {
  private var height: CGFloat
  init(presentedViewController: UIViewController, presenting presentingViewController: UIViewController?, presentationHeight: CGFloat) {
    height = presentationHeight
    super.init(presentedViewController: presentedViewController, presenting: presentingViewController)
  }
  
  required init(presentedViewController: UIViewController, presenting presentingViewController: UIViewController?, cancellable: Bool = true) {
    fatalError("init(presentedViewController:presenting:cancellable:) has not been implemented")
  }
  
  override var frameOfPresentedViewInContainerView: CGRect {
    let f = super.frameOfPresentedViewInContainerView
    return CGRect(x: 0, y: f.height - height, width: f.width, height: height)
  }
}
