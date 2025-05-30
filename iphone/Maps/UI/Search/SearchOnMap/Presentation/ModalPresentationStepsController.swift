final class ModalPresentationStepsController {

  enum StepUpdate {
    case didClose
    case didUpdateFrame(CGRect)
    case didUpdateStep(ModalPresentationStep)
  }

  fileprivate enum Constants {
    static let slowSwipeVelocity: CGFloat = 500
    static let fastSwipeDownVelocity: CGFloat = 4000
    static let fastSwipeUpVelocity: CGFloat = 3000
    static let translationThreshold: CGFloat = 50
  }

  private weak var presentedView: UIView?
  private weak var containerViewController: UIViewController?

  private var initialTranslationY: CGFloat = .zero

  private(set) var currentStep: ModalPresentationStep = .fullScreen
  private(set) var maxAvailableFrame: CGRect = .zero

  var currentFrame: CGRect { frame(for: currentStep) }
  var hiddenFrame: CGRect { frame(for: .hidden) }

  var didUpdateHandler: ((StepUpdate) -> Void)?

  func set(presentedView: UIView, containerViewController: UIViewController) {
    self.presentedView = presentedView
    self.containerViewController = containerViewController
  }

  func setInitialState() {
    setStep(.hidden, animation: .none)
  }

  func close(completion: (() -> Void)? = nil) {
    setStep(.hidden, animation: .slide, completion: completion)
  }

  func updateMaxAvailableFrame() {
    maxAvailableFrame = frame(for: .fullScreen)
  }

  func handlePan(_ gesture: UIPanGestureRecognizer) {
    guard let presentedView else { return }
    let translation = gesture.translation(in: presentedView)
    let velocity = gesture.velocity(in: presentedView)
    var currentFrame = presentedView.frame

    switch gesture.state {
    case .began:
      initialTranslationY = presentedView.frame.origin.y
    case .changed:
      let newY = max(max(initialTranslationY + translation.y, 0), maxAvailableFrame.origin.y)
      currentFrame.origin.y = newY
      presentedView.frame = currentFrame
      didUpdateHandler?(.didUpdateFrame(currentFrame))
    case .ended:
      let nextStep: ModalPresentationStep
      if velocity.y > Constants.fastSwipeDownVelocity {
        didUpdateHandler?(.didClose)
        return
      } else if velocity.y < -Constants.fastSwipeUpVelocity {
        nextStep = .fullScreen
      } else if velocity.y > Constants.slowSwipeVelocity || translation.y > Constants.translationThreshold {
        if currentStep == .compact {
          didUpdateHandler?(.didClose)
          return
        }
        nextStep = currentStep.lower
      } else if velocity.y < -Constants.slowSwipeVelocity || translation.y < -Constants.translationThreshold {
        nextStep = currentStep.upper
      } else {
        nextStep = currentStep
      }

      let animation: PresentationStepChangeAnimation = abs(velocity.y) > Constants.slowSwipeVelocity ? .slideAndBounce : .slide
      setStep(nextStep, animation: animation)
    default:
      break
    }
  }

  func setStep(_ step: ModalPresentationStep,
               completion: (() -> Void)? = nil) {
    guard currentStep != step else { return }
    setStep(step, animation: .slide, completion: completion)
  }

  private func setStep(_ step: ModalPresentationStep,
                       animation: PresentationStepChangeAnimation,
                       completion: (() -> Void)? = nil) {
    guard let presentedView else { return }
    currentStep = step
    updateMaxAvailableFrame()

    let frame = frame(for: step)
    didUpdateHandler?(.didUpdateStep(step))
    didUpdateHandler?(.didUpdateFrame(frame))

    ModalPresentationAnimator.animate(with: animation) {
      presentedView.frame = frame
    } completion: { _ in
      completion?()
    }
  }

  private func frame(for step: ModalPresentationStep) -> CGRect {
    guard let presentedView, let containerViewController else { return .zero }
    return step.frame(for: presentedView, in: containerViewController)
  }
}
