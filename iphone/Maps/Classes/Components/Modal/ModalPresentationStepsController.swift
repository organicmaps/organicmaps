protocol ModalPresentationStep: Equatable {
  static var expanded: Self { get }
  static var hidden: Self { get }
}

fileprivate enum Constants {
  static let slowSwipeVelocity: CGFloat = 500
  static let fastSwipeDownVelocity: CGFloat = 4000
  static let fastSwipeUpVelocity: CGFloat = 3000
  static let translationThreshold: CGFloat = 50
}

final class ModalPresentationStepsController<Step: ModalPresentationStep> {

  enum StepUpdate {
    case didClose
    case didUpdateFrame(CGRect)
    case didUpdateStep(Step)
  }

  private weak var presentedView: UIView?
  private weak var containerViewController: UIViewController?
  private var currentStep: Step
  private(set) var maxAvailableFrame: CGRect = .zero

  var stepStrategy: any ModalPresentationStepStrategy<Step>
  var didUpdateHandler: ((StepUpdate) -> Void)?
  var currentFrame: CGRect { frame(for: currentStep) }
  var hiddenFrame: CGRect { frame(for: .hidden) }

  private var initialTranslationY: CGFloat = .zero

  init(presentedView: UIView,
       containerViewController: UIViewController,
       stepStrategy: any ModalPresentationStepStrategy<Step>,
       currentStep: Step,
       didUpdateHandler: @escaping ((StepUpdate) -> Void)) {
    self.presentedView = presentedView
    self.containerViewController = containerViewController
    self.stepStrategy = stepStrategy
    self.currentStep = currentStep
    self.didUpdateHandler = didUpdateHandler
  }

  func setInitialState() {
    setStep(.hidden, animation: .none)
  }

  func close(completion: (() -> Void)? = nil) {
    setStep(.hidden, animation: .slide, completion: completion)
  }

  func updateFrame() {
    let newFrame = frame(for: currentStep)
    presentedView?.frame = newFrame
    didUpdateHandler?(.didUpdateFrame(newFrame))
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
      let nextStep: Step
      if velocity.y > Constants.fastSwipeDownVelocity {
        didUpdateHandler?(.didClose)
        return
      } else if velocity.y < -Constants.fastSwipeUpVelocity {
        nextStep = .expanded
      } else if velocity.y > Constants.slowSwipeVelocity || translation.y > Constants.translationThreshold {
        if stepStrategy.lowerTo(currentStep) == .hidden {
          didUpdateHandler?(.didClose)
          return
        }
        nextStep = stepStrategy.lowerTo(currentStep)
      } else if velocity.y < -Constants.slowSwipeVelocity || translation.y < -Constants.translationThreshold {
        nextStep = stepStrategy.upperTo(currentStep)
      } else {
        nextStep = currentStep
      }

      let animation: PresentationStepChangeAnimation = abs(velocity.y) > Constants.slowSwipeVelocity ? .slideAndBounce : .slide
      setStep(nextStep, animation: animation)
    default:
      break
    }
  }

  func setStep(_ step: Step,
               completion: (() -> Void)? = nil) {
    guard currentStep != step else { return }
    setStep(step, animation: .slide, completion: completion)
  }

  private func setStep(_ step: Step,
                       animation: PresentationStepChangeAnimation,
                       completion: (() -> Void)? = nil) {
    guard let presentedView else { return }
    currentStep = step

    let frame = frame(for: step)
    didUpdateHandler?(.didUpdateStep(step))
    didUpdateHandler?(.didUpdateFrame(frame))

    ModalPresentationAnimator.animate(with: animation) {
      presentedView.frame = frame
    } completion: { _ in
      completion?()
    }
  }

  private func frame(for step: Step) -> CGRect {
    guard let presentedView, let containerViewController else { return .zero }
    maxAvailableFrame = stepStrategy.frame(.expanded, for: presentedView, in: containerViewController)
    return stepStrategy.frame(step, for: presentedView, in: containerViewController)
  }
}
