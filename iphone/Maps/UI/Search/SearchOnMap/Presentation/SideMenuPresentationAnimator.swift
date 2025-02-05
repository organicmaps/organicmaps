final class SideMenuPresentationAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration / 2
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let toVC = transitionContext.viewController(forKey: .to) else { return }
    let containerView = transitionContext.containerView
    let finalFrame = transitionContext.finalFrame(for: toVC)
    let originFrame = finalFrame.offsetBy(dx: -finalFrame.width, dy: 0)
    containerView.addSubview(toVC.view)
    toVC.view.frame = originFrame
    toVC.view.autoresizingMask = [.flexibleWidth, .flexibleHeight]

    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   delay: .zero,
                   options: .curveEaseOut,
                   animations: {
      toVC.view.frame = finalFrame
    },
                   completion: {
      transitionContext.completeTransition($0)
    })
  }
}
