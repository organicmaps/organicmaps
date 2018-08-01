final class AlertDismissalAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let fromVC = transitionContext.viewController(forKey: .from) else { return }
    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: {
                    fromVC.view.alpha = 0
    }) { finished in
      fromVC.view.removeFromSuperview()
      transitionContext.completeTransition(finished)
    }
  }
}
