final class CoverVerticalModalTransitioning: NSObject, UIViewControllerTransitioningDelegate {
  private var height: CGFloat
  init(presentationHeight: CGFloat) {
    height = presentationHeight
  }

  func animationController(forPresented _: UIViewController,
                           presenting _: UIViewController,
                           source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    CoverVerticalPresentationAnimator()
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    CoverVerticalDismissalAnimator()
  }

  func presentationController(forPresented presented: UIViewController,
                              presenting: UIViewController?,
                              source _: UIViewController) -> UIPresentationController? {
    PresentationController(presentedViewController: presented, presenting: presenting, presentationHeight: height)
  }
}

private final class PresentationController: DimmedModalPresentationController {
  private var height: CGFloat
  init(presentedViewController: UIViewController, presenting presentingViewController: UIViewController?, presentationHeight: CGFloat) {
    height = presentationHeight
    super.init(presentedViewController: presentedViewController, presenting: presentingViewController)
  }

  required init(presentedViewController _: UIViewController, presenting _: UIViewController?, cancellable _: Bool = true) {
    fatalError("init(presentedViewController:presenting:cancellable:) has not been implemented")
  }

  override var frameOfPresentedViewInContainerView: CGRect {
    let f = super.frameOfPresentedViewInContainerView
    return CGRect(x: 0, y: f.height - height, width: f.width, height: height)
  }
}
