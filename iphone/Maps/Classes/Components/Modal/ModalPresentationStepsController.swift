protocol ModalPresentationStep: Equatable {
  static var expanded: Self { get }
  static var hidden: Self { get }
}

private enum Constants {
  static let slowSwipeVelocity: CGFloat = 500
  static let fastSwipeDownVelocity: CGFloat = 4000
  static let fastSwipeUpVelocity: CGFloat = 3000
  static let minRecognizedTranslation: CGFloat = 50
  static let maxRecognizedTranslation: CGFloat = 200
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
  private var isPanning: Bool = false

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
      isPanning = true
    case .changed:
      let newY = max(max(initialTranslationY + translation.y, 0), maxAvailableFrame.origin.y)
      currentFrame.origin.y = newY
      presentedView.frame = currentFrame
      didUpdateHandler?(.didUpdateFrame(currentFrame))
    case .ended:
      let nextStep: Step
      if velocity.y > Constants.fastSwipeDownVelocity {
        nextStep = .hidden
      } else if velocity.y < -Constants.fastSwipeUpVelocity {
        nextStep = .expanded
      } else if abs(translation.y) > Constants.maxRecognizedTranslation {
        nextStep = nearestStep(for: currentFrame.origin.y)
      } else if velocity.y > Constants.slowSwipeVelocity && translation.y > Constants.minRecognizedTranslation {
        if stepStrategy.lowerTo(currentStep) == .hidden {
          nextStep = .hidden
        } else {
          nextStep = stepStrategy.lowerTo(currentStep)
        }
      } else if velocity.y < -Constants.slowSwipeVelocity && translation.y < -Constants.minRecognizedTranslation {
        nextStep = stepStrategy.upperTo(currentStep)
      } else {
        nextStep = nearestStep(for: currentFrame.origin.y)
      }

      guard nextStep != .hidden else {
        didUpdateHandler?(.didClose)
        return
      }

      let animation: PresentationStepChangeAnimation = abs(velocity.y) > Constants.slowSwipeVelocity ? .slideAndBounce : .slide
      setStep(nextStep, animation: animation)
    default:
      break
    }
  }

  func setStep(_ step: Step, forced: Bool = false, completion: (() -> Void)? = nil) {
    guard currentStep != step || (forced && !isPanning) else { return }
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
    } completion: { [weak self] _ in
      self?.isPanning = false
      completion?()
    }
  }

  private func frame(for step: Step) -> CGRect {
    guard let presentedView, let containerViewController else { return .zero }
    maxAvailableFrame = stepStrategy.frame(.expanded, for: presentedView, in: containerViewController)
    return stepStrategy.frame(step, for: presentedView, in: containerViewController)
  }

  private func nearestStep(for positionY: CGFloat) -> Step {
    let visibleSteps = stepStrategy.steps.filter { $0 != .hidden }
    guard !visibleSteps.isEmpty else { return currentStep }

    var bestStep = visibleSteps.contains(currentStep) ? currentStep : visibleSteps[0]
    var bestDistance = abs(frame(for: bestStep).origin.y - positionY)

    for step in visibleSteps where step != bestStep {
      let distance = abs(frame(for: step).origin.y - positionY)
      if distance < bestDistance {
        bestStep = step
        bestDistance = distance
      }
    }

    return bestStep
  }
}
