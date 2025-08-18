final class RouteManagerTransitioning: NSObject, UIViewControllerAnimatedTransitioning {
  private let isPresentation: Bool

  init(isPresentation: Bool) {
    self.isPresentation = isPresentation
    super.init()
  }

  func transitionDuration(using _: UIViewControllerContextTransitioning?) -> TimeInterval {
    return kDefaultAnimationDuration
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    guard let fromVC = transitionContext.viewController(forKey: .from),
      let toVC = transitionContext.viewController(forKey: .to) else { return }

    let animatingVC = isPresentation ? toVC : fromVC
    guard let animatingView = animatingVC.view else { return }

    let finalFrameForVC = transitionContext.finalFrame(for: animatingVC)
    var initialFrameForVC = finalFrameForVC
    initialFrameForVC.origin.y += initialFrameForVC.size.height

    let initialFrame = isPresentation ? initialFrameForVC : finalFrameForVC
    let finalFrame = isPresentation ? finalFrameForVC : initialFrameForVC

    animatingView.frame = initialFrame

    UIView.animate(withDuration: transitionDuration(using: transitionContext),
                   animations: { animatingView.frame = finalFrame },
                   completion: { _ in
                     transitionContext.completeTransition(true)
    })
  }
}
