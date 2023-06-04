final class CoverVerticalPresentationAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let toVC = transitionContext.viewController(forKey: .to) else { return }

    let containerView = transitionContext.containerView
    let finalFrame = transitionContext.finalFrame(for: toVC)
    let originFrame = finalFrame.offsetBy(dx: 0, dy: finalFrame.height)

    containerView.addSubview(toVC.view)
    toVC.view.frame = originFrame
    toVC.view.autoresizingMask = [.flexibleWidth, .flexibleTopMargin]
    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: {
                    toVC.view.frame = finalFrame
    }) { transitionContext.completeTransition($0) }
  }
}
