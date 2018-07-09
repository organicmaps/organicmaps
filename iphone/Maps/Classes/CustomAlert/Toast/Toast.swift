@objc(MWMToast)
@objcMembers
final class Toast: NSObject {

  private var blurView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))

  static func toast(withText text: String) -> Toast {
    return Toast(text)
  }

  private init(_ text: String) {
    blurView.layer.cornerRadius = 8
    blurView.clipsToBounds = true
    blurView.alpha = 0

    let label = UILabel()
    label.text = text
    label.textAlignment = .center
    label.numberOfLines = 0
    label.font = .regular14()
    label.textColor = .white
    label.frame = blurView.contentView.bounds
    label.translatesAutoresizingMaskIntoConstraints = false
    blurView.contentView.addSubview(label)
    let views = ["label" : label]
    blurView.contentView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|-8-[label]-8-|",
                                                                       options: [],
                                                                       metrics: [:],
                                                                       views: views))
    blurView.contentView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:|-8-[label]-8-|",
                                                                       options: [],
                                                                       metrics: [:],
                                                                       views: views))
  }

  func show() {
    show(in: UIApplication.shared.keyWindow)
  }

  func show(in view: UIView?) {
    guard let v = view else { return }
    blurView.translatesAutoresizingMaskIntoConstraints = false
    v.addSubview(blurView)
    let views = ["bv" : blurView]
    v.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|->=16-[bv]->=16-|",
                                                    options: [],
                                                    metrics: [:],
                                                    views: views))
    v.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:[bv]-30-|",
                                                    options: [],
                                                    metrics: [:],
                                                    views: views))
    v.addConstraint(NSLayoutConstraint(item: blurView,
                                       attribute: .centerX,
                                       relatedBy: .equal,
                                       toItem: v,
                                       attribute: .centerX,
                                       multiplier: 1,
                                       constant: 0))

    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.blurView.alpha = 1
    }

    Timer.scheduledTimer(timeInterval: 3,
                         target: self,
                         selector: #selector(onTimer),
                         userInfo: nil,
                         repeats: false)
  }

  @objc private func onTimer() {
    UIView.animate(withDuration: kDefaultAnimationDuration,
                   animations: { self.blurView.alpha = 0 }) { [self] _ in self.blurView.removeFromSuperview() }
  }
}
