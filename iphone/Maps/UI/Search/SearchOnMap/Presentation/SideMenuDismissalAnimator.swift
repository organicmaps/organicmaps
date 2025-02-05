final class SideMenuDismissalAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration / 2
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let fromVC = transitionContext.viewController(forKey: .from) else { return }
    let initialFrame = transitionContext.initialFrame(for: fromVC)
    let targetFrame = initialFrame.offsetBy(dx: -initialFrame.width, dy: 0)

    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   delay: .zero,
                   options: .curveEaseIn,
                   animations: {
      fromVC.view.frame = targetFrame
    },
                   completion: {
      fromVC.view.removeFromSuperview()
      transitionContext.completeTransition($0)
    })
  }
}
