final class FadeInAnimatedTransitioning: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using _: UIViewControllerContextTransitioning?) -> TimeInterval {
    kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let presentedView = transitionContext.view(forKey: .to) else { return }

    presentedView.alpha = 0
    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: {
                     presentedView.alpha = 1
                   }) { transitionContext.completeTransition($0) }
  }
}
