import UIKit

final class PhotosTransitionAnimator: NSObject, UIViewControllerAnimatedTransitioning {
  private enum Settings {
    static let animationDurationWithZooming = 2 * kDefaultAnimationDuration
    static let animationDurationWithoutZooming = kDefaultAnimationDuration
    static let animationDurationEndingViewFadeInRatio = 0.1
    static let animationDurationStartingViewFadeOutRatio = 0.05
    static let zoomingAnimationSpringDamping: CGFloat = 0.9
    static let animationDurationFadeRatio = 4.0 / 9.0
  }

  var dismissing = false

  var startingView: UIView?
  var endingView: UIView?

  private var shouldPerformZoomingAnimation: Bool {
    return startingView != nil && endingView != nil
  }

  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    setupTransitionContainerHierarchyWithTransitionContext(transitionContext)
    if shouldPerformZoomingAnimation {
      performZoomingAnimationWithTransitionContext(transitionContext)
    }
    performFadeAnimationWithTransitionContext(transitionContext)
  }

  func transitionDuration(using _: UIViewControllerContextTransitioning?) -> TimeInterval {
    return shouldPerformZoomingAnimation ? Settings.animationDurationWithZooming : Settings.animationDurationWithoutZooming
  }

  private func fadeDurationForTransitionContext(_ transitionContext: UIViewControllerContextTransitioning) -> TimeInterval {
    let transDuration = transitionDuration(using: transitionContext)
    return shouldPerformZoomingAnimation ? transDuration * Settings.animationDurationFadeRatio : transDuration
  }

  private func setupTransitionContainerHierarchyWithTransitionContext(_ transitionContext: UIViewControllerContextTransitioning) {
    if let toView = transitionContext.view(forKey: UITransitionContextViewKey.to),
      let toViewController = transitionContext.viewController(forKey: UITransitionContextViewControllerKey.to) {
      toView.frame = transitionContext.finalFrame(for: toViewController)
      let containerView = transitionContext.containerView

      if !toView.isDescendant(of: containerView) {
        containerView.addSubview(toView)
      }
    }

    if dismissing {
      if let fromView = transitionContext.view(forKey: UITransitionContextViewKey.from) {
        transitionContext.containerView.bringSubview(toFront: fromView)
      }
    }
  }

  private func performZoomingAnimationWithTransitionContext(_ transitionContext: UIViewControllerContextTransitioning) {
    let containerView = transitionContext.containerView
    guard let startingView = startingView, let endingView = endingView else { return }

    let startingViewForAnimation = startingView.snapshot
    let endingViewForAnimation = endingView.snapshot

    let finalEndingViewTransform = endingView.transform
    let endingViewInitialTransform = startingViewForAnimation.frame.height / endingViewForAnimation.frame.height
    let startingViewFinalTransform = 1.0 / endingViewInitialTransform

    let translatedStartingViewCenter = startingView.center(inContainerView: containerView)
    let translatedEndingViewFinalCenter = endingView.center(inContainerView: containerView)

    startingViewForAnimation.center = translatedStartingViewCenter

    endingViewForAnimation.transform = endingViewForAnimation.transform.scaledBy(x: endingViewInitialTransform, y: endingViewInitialTransform)
    endingViewForAnimation.center = translatedStartingViewCenter
    endingViewForAnimation.alpha = 0.0

    containerView.addSubview(startingViewForAnimation)
    containerView.addSubview(endingViewForAnimation)

    endingView.alpha = 0.0
    startingView.alpha = 0.0

    let transDuration = transitionDuration(using: transitionContext)
    let fadeInDuration = transDuration * Settings.animationDurationEndingViewFadeInRatio
    let fadeOutDuration = transDuration * Settings.animationDurationStartingViewFadeOutRatio

    UIView.animate(withDuration: fadeInDuration,
                   delay: 0.0,
                   options: [.allowAnimatedContent, .beginFromCurrentState],
                   animations: { endingViewForAnimation.alpha = 1.0 },
                   completion: { _ in
                     UIView.animate(withDuration: fadeOutDuration,
                                    delay: 0.0,
                                    options: [.allowAnimatedContent, .beginFromCurrentState],
                                    animations: { startingViewForAnimation.alpha = 0.0 },
                                    completion: { _ in
                                      startingViewForAnimation.removeFromSuperview()
                     })

    })

    UIView.animate(withDuration: transDuration,
                   delay: 0.0,
                   usingSpringWithDamping: Settings.zoomingAnimationSpringDamping,
                   initialSpringVelocity: 0,
                   options: [.allowAnimatedContent, .beginFromCurrentState],
                   animations: { () -> Void in
                     endingViewForAnimation.transform = finalEndingViewTransform
                     endingViewForAnimation.center = translatedEndingViewFinalCenter
                     startingViewForAnimation.transform = startingViewForAnimation.transform.scaledBy(x: startingViewFinalTransform, y: startingViewFinalTransform)
                     startingViewForAnimation.center = translatedEndingViewFinalCenter
                   },
                   completion: { [weak self] _ in
                     endingViewForAnimation.removeFromSuperview()
                     endingView.alpha = 1.0
                     startingView.alpha = 1.0
                     self?.completeTransitionWithTransitionContext(transitionContext)
    })
  }

  private func performFadeAnimationWithTransitionContext(_ transitionContext: UIViewControllerContextTransitioning) {
    let fadeView = dismissing ? transitionContext.view(forKey: UITransitionContextViewKey.from) : transitionContext.view(forKey: UITransitionContextViewKey.to)
    let beginningAlpha: CGFloat = dismissing ? 1.0 : 0.0
    let endingAlpha: CGFloat = dismissing ? 0.0 : 1.0

    fadeView?.alpha = beginningAlpha

    UIView.animate(withDuration: fadeDurationForTransitionContext(transitionContext), animations: {
      fadeView?.alpha = endingAlpha
    }) { _ in
      if !self.shouldPerformZoomingAnimation {
        self.completeTransitionWithTransitionContext(transitionContext)
      }
    }
  }

  private func completeTransitionWithTransitionContext(_ transitionContext: UIViewControllerContextTransitioning) {
    if transitionContext.isInteractive {
      if transitionContext.transitionWasCancelled {
        transitionContext.cancelInteractiveTransition()
      } else {
        transitionContext.finishInteractiveTransition()
      }
    }
    transitionContext.completeTransition(!transitionContext.transitionWasCancelled)
  }
}
