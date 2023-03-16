final class CoverVerticalDismissalAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let fromVC = transitionContext.viewController(forKey: .from),
      let toVC = transitionContext.viewController(forKey: .to)
      else { return }

    let originFrame = transitionContext.finalFrame(for: toVC)
    let finalFrame = originFrame.offsetBy(dx: 0, dy: originFrame.height)
    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: {
                    fromVC.view.frame = finalFrame
    }) { finished in
      fromVC.view.removeFromSuperview()
      transitionContext.completeTransition(finished)
    }
  }
}
