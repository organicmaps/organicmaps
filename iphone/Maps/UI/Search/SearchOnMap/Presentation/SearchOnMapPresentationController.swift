protocol ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat)
}

final class SearchOnMapPresentationController: NSObject {

  private enum StepChangeAnimation {
    case none
    case slide
    case slideAndBounce
  }

  private enum Constants {
    static let animationDuration: TimeInterval = kDefaultAnimationDuration
    static let springDamping: CGFloat = 0.85
    static let springVelocity: CGFloat = 0.2
    static let iPhoneCornerRadius: CGFloat = 10
    static let slowSwipeVelocity: CGFloat = 500
    static let fastSwipeDownVelocity: CGFloat = 4000
    static let fastSwipeUpVelocity: CGFloat = 3000
    static let translationThreshold: CGFloat = 50
    static let panGestureThreshold: CGFloat = 5
  }

  private var initialTranslationY: CGFloat = .zero
  private weak var interactor: SearchOnMapInteractor? { presentedViewController?.interactor }
  // TODO: (KK) replace with set of steps passed from the outside
  private var presentationStep: ModalScreenPresentationStep = .fullScreen
  private var internalScrollViewContentOffset: CGFloat = .zero
  private var maxAvailableFrameOfPresentedView: CGRect = .zero

  private weak var presentedViewController: SearchOnMapViewController?
  private weak var parentViewController: UIViewController?
  private weak var containerView: UIView?

  init(parentViewController: UIViewController,
       containerView: UIView) {
    self.parentViewController = parentViewController
    self.containerView = containerView
  }

  func setViewController(_ viewController: SearchOnMapViewController) {
    self.presentedViewController = viewController
    guard let containerView, let parentViewController else { return }
    containerView.addSubview(viewController.view)
    parentViewController.addChild(viewController)
    viewController.view.frame = frameOfPresentedViewInContainerView
    viewController.didMove(toParent: parentViewController)

    iPhoneSpecific {
      let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
      panGestureRecognizer.delegate = self
      viewController.view.addGestureRecognizer(panGestureRecognizer)
      viewController.scrollViewDelegate = self
    }
    animateTo(.hidden, animation: .none)
  }

  func show() {
    interactor?.handle(.openSearch)
  }

  func close() {
    guard let presentedViewController else { return }
    presentedViewController.willMove(toParent: nil)
    animateTo(.hidden) {
      presentedViewController.view.removeFromSuperview()
      presentedViewController.removeFromParent()
    }
  }

  func setPresentationStep(_ step: ModalScreenPresentationStep) {
    guard presentationStep != step else { return }
    animateTo(step)
  }

  // MARK: - Layout
  func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    presentedViewController?.view.frame = frameOfPresentedViewInContainerView
    presentedViewController?.view.layoutIfNeeded()
  }

  private var frameOfPresentedViewInContainerView: CGRect {
    updateMaxAvailableFrameOfPresentedView()
    let frame = presentationStep.frame()
    return frame
  }

  private func updateMaxAvailableFrameOfPresentedView() {
    maxAvailableFrameOfPresentedView = ModalScreenPresentationStep.fullScreen.frame()
  }

  private func updateSideButtonsAvailableArea(_ newY: CGFloat) {
    guard presentedViewController?.traitCollection.verticalSizeClass != .compact else { return }
    var sideButtonsAvailableArea = MWMSideButtons.getAvailableArea()
    sideButtonsAvailableArea.size.height = newY - sideButtonsAvailableArea.origin.y
    MWMSideButtons.updateAvailableArea(sideButtonsAvailableArea)
  }

  // MARK: - Pan gesture handling
  @objc private func handlePan(_ gesture: UIPanGestureRecognizer) {
    guard let presentedViewController, let presentedView = presentedViewController.view else { return }
    interactor?.handle(.didStartDraggingSearch)

    let translation = gesture.translation(in: presentedView)
    let velocity = gesture.velocity(in: presentedView)

    switch gesture.state {
    case .began:
      initialTranslationY = presentedView.frame.origin.y
    case .changed:
      let newY = max(max(initialTranslationY + translation.y, 0), maxAvailableFrameOfPresentedView.origin.y)
      presentedView.frame.origin.y = newY
      translationYDidUpdate(newY)
    case .ended:
      let nextStep: ModalScreenPresentationStep
      if velocity.y > Constants.fastSwipeDownVelocity {
        interactor?.handle(.closeSearch)
        return
      } else if velocity.y < -Constants.fastSwipeUpVelocity {
        nextStep = .fullScreen // fast swipe up
      } else if velocity.y > Constants.slowSwipeVelocity || translation.y > Constants.translationThreshold {
        if presentationStep == .compact {
          interactor?.handle(.closeSearch)
          return
        }
        nextStep = presentationStep.lower // regular swipe down
      } else if velocity.y < -Constants.slowSwipeVelocity || translation.y < -Constants.translationThreshold {
        nextStep = presentationStep.upper // regular swipe up
      } else {
        // TODO: swipe to closest step on the big translation
        nextStep = presentationStep
      }
      let animation: StepChangeAnimation = abs(velocity.y) > Constants.slowSwipeVelocity ? .slideAndBounce : .slide
      animateTo(nextStep, animation: animation)
    default:
      break
    }
  }

  private func animateTo(_ presentationStep: ModalScreenPresentationStep, animation: StepChangeAnimation = .slide, completion: (() -> Void)? = nil) {
    guard let presentedViewController, let presentedView = presentedViewController.view else { return }
    self.presentationStep = presentationStep
    interactor?.handle(.didUpdatePresentationStep(presentationStep))

    let updatedFrame = presentationStep.frame()
    let targetYTranslation = updatedFrame.origin.y

    switch animation {
    case .none:
      presentedView.frame = updatedFrame
      translationYDidUpdate(targetYTranslation)
      completion?()
    case .slide:
      UIView.animate(withDuration: Constants.animationDuration,
                     delay: 0,
                     options: .curveEaseOut,
                     animations: { [weak self] in
        presentedView.frame = updatedFrame
        self?.translationYDidUpdate(targetYTranslation)
      }) { _ in
        completion?()
      }
    case .slideAndBounce:
      UIView.animate(withDuration: Constants.animationDuration,
                     delay: 0,
                     usingSpringWithDamping: Constants.springDamping,
                     initialSpringVelocity: Constants.springVelocity,
                     options: .curveLinear,
                     animations: { [weak self] in
        presentedView.frame = updatedFrame
        self?.translationYDidUpdate(targetYTranslation)
      }) { _ in
        completion?()
      }
    }
  }
}

// MARK: - ModallyPresentedViewController
extension SearchOnMapPresentationController: ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat) {
    iPhoneSpecific {
      presentedViewController?.translationYDidUpdate(translationY)
      updateSideButtonsAvailableArea(translationY)
    }
  }
}

// MARK: - UIGestureRecognizerDelegate
extension SearchOnMapPresentationController: UIGestureRecognizerDelegate {
  func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    true
  }

  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    // threshold is used to soften transition from the internal scroll zero content offset
    internalScrollViewContentOffset < Constants.panGestureThreshold
  }
}

// MARK: - SearchOnMapScrollViewDelegate
extension SearchOnMapPresentationController: SearchOnMapScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    guard let presentedViewController, let presentedView = presentedViewController.view else { return }
    let hasReachedTheTop = Int(presentedView.frame.origin.y) > Int(maxAvailableFrameOfPresentedView.origin.y)
    let hasZeroContentOffset = internalScrollViewContentOffset == 0
    if hasReachedTheTop && hasZeroContentOffset {
      // prevent the internal scroll view scrolling
      scrollView.contentOffset.y = internalScrollViewContentOffset
      return
    }
    internalScrollViewContentOffset = scrollView.contentOffset.y
  }
}
