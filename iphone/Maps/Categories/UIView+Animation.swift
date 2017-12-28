import UIKit

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
}
