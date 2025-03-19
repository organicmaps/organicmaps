enum PresentationStepChangeAnimation {
  case none
  case slide
  case slideAndBounce
}

final class ModalPresentationAnimator {

  private enum Constants {
    static let animationDuration: TimeInterval = kDefaultAnimationDuration
    static let springDamping: CGFloat = 0.85
    static let springVelocity: CGFloat = 0.2
  }

  static func animate(with stepAnimation: PresentationStepChangeAnimation = .slide,
                      animations: @escaping (() -> Void),
                      completion: ((Bool) -> Void)?) {
    switch stepAnimation {
    case .none:
      animations()
      completion?(true)
    case .slide:
      UIView.animate(withDuration: Constants.animationDuration,
                     delay: 0,
                     options: .curveEaseOut,
                     animations: animations,
                     completion: completion)
    case .slideAndBounce:
      UIView.animate(withDuration: Constants.animationDuration,
                     delay: 0,
                     usingSpringWithDamping: Constants.springDamping,
                     initialSpringVelocity: Constants.springVelocity,
                     options: .curveLinear,
                     animations: animations,
                     completion: completion)
    }
  }
}
