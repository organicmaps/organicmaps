enum PresentationStepChangeAnimation {
  case none
  case slide
  case slideAndBounce
}

final class ModalPresentationAnimator {

  private enum Constants {
    static let animationDuration: TimeInterval = kDefaultAnimationDuration
    static let springDamping: CGFloat = 0.8
    static let springVelocity: CGFloat = 0.2
    static let controlPoint1: CGPoint = CGPoint(x: 0.25, y: 0.1)
    static let controlPoint2: CGPoint = CGPoint(x: 0.15, y: 1.0)
  }

  static func animate(with stepAnimation: PresentationStepChangeAnimation = .slide,
                      animations: @escaping (() -> Void),
                      completion: ((Bool) -> Void)?) {
    switch stepAnimation {
    case .none:
      animations()
      completion?(true)

    case .slide:
      let timing = UICubicTimingParameters(controlPoint1: Constants.controlPoint1,
                                           controlPoint2: Constants.controlPoint2)
      let animator = UIViewPropertyAnimator(duration: Constants.animationDuration,
                                            timingParameters: timing)
      animator.addAnimations(animations)
      animator.addCompletion { position in
        completion?(position == .end)
      }
      animator.startAnimation()

    case .slideAndBounce:
      let velocity = CGVector(dx: Constants.springVelocity, dy: Constants.springVelocity)
      let timing = UISpringTimingParameters(dampingRatio: Constants.springDamping,
                                            initialVelocity: velocity)
      let animator = UIViewPropertyAnimator(duration: Constants.animationDuration,
                                            timingParameters: timing)
      animator.addAnimations(animations)
      animator.addCompletion { position in
        completion?(position == .end)
      }
      animator.startAnimation()
    }
  }
}
