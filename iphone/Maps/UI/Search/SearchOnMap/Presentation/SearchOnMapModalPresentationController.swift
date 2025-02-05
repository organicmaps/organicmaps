protocol ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat)
}

protocol SearchOnMapModalPresentationView: AnyObject {
  func setPresentationStep(_ step: ModalScreenPresentationStep)
  func close()
}

final class SearchOnMapModalPresentationController: UIPresentationController {

  private enum StepChangeAnimation {
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

  private var initialTranslationY: CGFloat = 0
  private weak var interactor: SearchOnMapInteractor? {
    (presentedViewController as? SearchOnMapViewController)?.interactor
  }
  // TODO: replace with set of steps passed from the outside
  private var presentationStep: ModalScreenPresentationStep = .fullScreen
  private var internalScrollViewContentOffset: CGFloat = 0
  private var maxAvailableFrameOfPresentedView: CGRect = .zero

  // MARK: - Init
  override init(presentedViewController: UIViewController, presenting presentingViewController: UIViewController?) {
    super.init(presentedViewController: presentedViewController, presenting: presentingViewController)

    iPhoneSpecific {
      let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
      panGestureRecognizer.delegate = self
      presentedViewController.view.addGestureRecognizer(panGestureRecognizer)
      if let presentedViewController = presentedViewController as? SearchOnMapView {
        presentedViewController.scrollViewDelegate = self
      }
    }
  }

  // MARK: - Lifecycle
  override func containerViewWillLayoutSubviews() {
    super.containerViewWillLayoutSubviews()
    presentedView?.frame = frameOfPresentedViewInContainerView
  }

  override func presentationTransitionWillBegin() {
    guard let containerView else { return }
    containerView.backgroundColor = .clear
    let passThroughView = MapPassthroughView(passingView: containerView)
    containerView.addSubview(passThroughView)
  }

  override func presentationTransitionDidEnd(_ completed: Bool) {
    translationYDidUpdate(presentedView?.frame.origin.y ?? 0)
  }

  override func dismissalTransitionDidEnd(_ completed: Bool) {
    super.dismissalTransitionDidEnd(completed)
    if completed {
      presentedView?.removeFromSuperview()
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateMaxAvailableFrameOfPresentedView()
  }

  // MARK: - Layout
  override var frameOfPresentedViewInContainerView: CGRect {
    guard let containerView else { return .zero }
    let frame = presentationStep.frame(for: presentedViewController, in: containerView)
    updateMaxAvailableFrameOfPresentedView()
    return frame
  }

  private func updateMaxAvailableFrameOfPresentedView() {
    guard let containerView else { return }
    maxAvailableFrameOfPresentedView = ModalScreenPresentationStep.fullScreen.frame(for: presentedViewController, in: containerView)
  }

  private func updateSideButtonsAvailableArea(_ newY: CGFloat) {
    iPhoneSpecific {
      guard presentedViewController.traitCollection.verticalSizeClass != .compact else { return }
      var sideButtonsAvailableArea = MWMSideButtons.getAvailableArea()
      sideButtonsAvailableArea.size.height = newY - sideButtonsAvailableArea.origin.y
      MWMSideButtons.updateAvailableArea(sideButtonsAvailableArea)
    }
  }

  // MARK: - Pan gesture handling
  @objc private func handlePan(_ gesture: UIPanGestureRecognizer) {
    guard let presentedView, maxAvailableFrameOfPresentedView != .zero else { return }
    interactor?.handle(.didStartDraggingSearch)

    let translation = gesture.translation(in: presentedView)
    let velocity = gesture.velocity(in: presentedView)

    switch gesture.state {
    case .began:
      initialTranslationY = presentedView.frame.origin.y
    case .changed:
      let newY = max(max(initialTranslationY + translation.y, 0), maxAvailableFrameOfPresentedView.origin.y)
      presentedView.frame.origin.y = newY
      updateSideButtonsAvailableArea(newY)
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

  private func animateTo(_ presentationStep: ModalScreenPresentationStep, animation: StepChangeAnimation = .slide) {
    guard let presentedView, let containerView else { return }
    self.presentationStep = presentationStep
    interactor?.handle(.didUpdatePresentationStep(presentationStep))

    let updatedFrame = presentationStep.frame(for: presentedViewController, in: containerView)
    let targetYTranslation = updatedFrame.origin.y

    switch animation {
    case .slide:
      UIView.animate(withDuration: Constants.animationDuration,
                     delay: 0,
                     options: .curveEaseOut,
                     animations: { [weak self] in
        presentedView.frame = updatedFrame
        self?.translationYDidUpdate(targetYTranslation)
        self?.updateSideButtonsAvailableArea(targetYTranslation)
      })
    case .slideAndBounce:
      UIView.animate(withDuration: Constants.animationDuration,
                     delay: 0,
                     usingSpringWithDamping: Constants.springDamping,
                     initialSpringVelocity: Constants.springVelocity,
                     options: .curveLinear,
                     animations: { [weak self] in
        presentedView.frame = updatedFrame
        self?.translationYDidUpdate(targetYTranslation)
        self?.updateSideButtonsAvailableArea(targetYTranslation)
      })
    }
  }
}

// MARK: - SearchOnMapModalPresentationView
extension SearchOnMapModalPresentationController: SearchOnMapModalPresentationView {
  func setPresentationStep(_ step: ModalScreenPresentationStep) {
    guard presentationStep != step else { return }
    animateTo(step)
  }

  func close() {
    guard let containerView else { return }
    updateSideButtonsAvailableArea(containerView.frame.height)
    presentedViewController.dismiss(animated: true)
  }
}

// MARK: - ModallyPresentedViewController
extension SearchOnMapModalPresentationController: ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat) {
    iPhoneSpecific {
      (presentedViewController as? SearchOnMapViewController)?.translationYDidUpdate(translationY)
    }
  }
}

// MARK: - UIGestureRecognizerDelegate
extension SearchOnMapModalPresentationController: UIGestureRecognizerDelegate {
  func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    true
  }

  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    // threshold is used to soften transition from the internal scroll zero content offset
    internalScrollViewContentOffset < Constants.panGestureThreshold
  }
}

// MARK: - SearchOnMapScrollViewDelegate
extension SearchOnMapModalPresentationController: SearchOnMapScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    guard let presentedView else { return }
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
