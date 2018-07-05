final class ModalTransitioning: NSObject, UIViewControllerTransitioningDelegate {
  func animationController(forPresented presented: UIViewController,
                           presenting: UIViewController,
                           source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return PresentationAnimator()
  }

  func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return DismissalAnimator()
  }

  func presentationController(forPresented presented: UIViewController,
                              presenting: UIViewController?,
                              source: UIViewController) -> UIPresentationController? {
    return PresentationController(presentedViewController: presented, presenting: presenting)
  }
}

fileprivate final class PresentationController: UIPresentationController {
  private lazy var onTapGr: UITapGestureRecognizer = {
    return UITapGestureRecognizer(target: self, action: #selector(onTap))
  }()

  private lazy var dimView: UIView = {
    let view = UIView()
    view.backgroundColor = UIColor.blackStatusBarBackground()
    view.addGestureRecognizer(onTapGr)
    return view
  }()

  @objc private func onTap() {
    presentingViewController.dismiss(animated: true, completion: nil)
  }

  override var frameOfPresentedViewInContainerView: CGRect {
    let f = super.frameOfPresentedViewInContainerView
    let h: CGFloat = 150;
    return CGRect(x: 0, y: f.height - h, width: f.width, height: h)
  }

  override func presentationTransitionWillBegin() {
    guard let containerView = containerView else { return }
    containerView.addSubview(dimView)
    dimView.frame = containerView.bounds
    dimView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    dimView.alpha = 0
    presentingViewController.transitionCoordinator?.animate(alongsideTransition: { _ in
      self.dimView.alpha = 1
    })
  }

  override func presentationTransitionDidEnd(_ completed: Bool) {
    if !completed {
      dimView.removeFromSuperview()
    }
  }

  override func dismissalTransitionWillBegin() {
    presentingViewController.transitionCoordinator?.animate(alongsideTransition: { _ in
      self.dimView.alpha = 0
    })
  }

  override func dismissalTransitionDidEnd(_ completed: Bool) {
    if completed { dimView.removeFromSuperview() }
  }
}

fileprivate final class PresentationAnimator: NSObject, UIViewControllerAnimatedTransitioning {
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
    }) { finished in
      transitionContext.completeTransition(finished)
    }
  }
}

fileprivate final class DismissalAnimator: NSObject, UIViewControllerAnimatedTransitioning {
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
