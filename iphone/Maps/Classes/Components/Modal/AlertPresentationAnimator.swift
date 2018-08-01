final class AlertPresentationAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let toVC = transitionContext.viewController(forKey: .to) else { return }

    let containerView = transitionContext.containerView
    let finalFrame = transitionContext.finalFrame(for: toVC)
    containerView.addSubview(toVC.view)

    toVC.view.alpha = 0
    toVC.view.center = containerView.center
    toVC.view.frame = finalFrame
    toVC.view.autoresizingMask = [.flexibleLeftMargin, .flexibleTopMargin, .flexibleRightMargin, .flexibleBottomMargin]
    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: {
                    toVC.view.alpha = 1
    }) { transitionContext.completeTransition($0) }
  }
}
