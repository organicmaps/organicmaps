extension UIView {
  func setHidden(_ hidden: Bool, animated: Bool = true, completion: (() -> Void)? = nil) {
    UIView.transition(with: self,
                      duration: animated ? kDefaultAnimationDuration : 0,
                      options: [.transitionCrossDissolve, .curveEaseInOut]) {
      self.alpha = hidden ? 0.0 : 1.0
      self.isHidden = hidden
    } completion: { _ in
      completion?()
    }
  }
}
