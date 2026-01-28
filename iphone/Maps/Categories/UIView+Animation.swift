
extension UIView {
  @objc func animateConstraints(duration: TimeInterval,
                                animations: @escaping () -> Void,
                                completion: @escaping () -> Void) {
    setNeedsLayout()
    UIView.animate(withDuration: duration,
                   animations: { [weak self] in
                     animations()
                     self?.layoutIfNeeded()
                   },
                   completion: { _ in completion() })
  }

  @objc func animateConstraints(animations: @escaping () -> Void, completion: @escaping () -> Void) {
    animateConstraints(duration: kDefaultAnimationDuration, animations: animations, completion: completion)
  }

  @objc func animateConstraints(animations: @escaping () -> Void) {
    animateConstraints(duration: kDefaultAnimationDuration, animations: animations, completion: {})
  }

  @objc func startRotation(_ duration: TimeInterval = 1.0) {
    let rotationAnimation = CABasicAnimation(keyPath: "transform.rotation.z")
    rotationAnimation.toValue = Double.pi * 2
    rotationAnimation.duration = duration;
    rotationAnimation.isCumulative = true;
    rotationAnimation.repeatCount = Float.greatestFiniteMagnitude;
    rotationAnimation.isRemovedOnCompletion = false
    layer.add(rotationAnimation, forKey: "rotationAnimation")
  }

  @objc func stopRotation() {
    layer.removeAnimation(forKey: "rotationAnimation")
  }

  @objc var isRotating: Bool {
    return layer.animationKeys()?.contains("rotationAnimation") ?? false
  }
}
