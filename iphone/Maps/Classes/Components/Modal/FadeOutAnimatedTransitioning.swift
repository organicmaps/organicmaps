final class FadeOutAnimatedTransitioning: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let presentedView = transitionContext.view(forKey: .from) else { return }
    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: {
                    presentedView.alpha = 0
    }) { finished in
      transitionContext.completeTransition(finished)
    }
  }
}
