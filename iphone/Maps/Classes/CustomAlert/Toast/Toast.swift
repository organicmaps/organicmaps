@objc(MWMToast)
final class Toast: NSObject {
  @objc(MWMToastAlignment)
  enum Alignment: Int {
    case bottom
    case top
  }

  private var blurView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
  private var timer: Timer?

  private static var toasts: [Toast] = []

  @objc static func toast(withText text: String) -> Toast {
    let toast = Toast(text)
    toasts.append(toast)
    return toast
  }
  
  @objc static func hideAll() {
    toasts.forEach { $0.hide() }
  }
  
  private init(_ text: String) {
    blurView.layer.setCorner(radius: 8)
    blurView.clipsToBounds = true
    blurView.alpha = 0

    let label = UILabel()
    label.text = text
    label.textAlignment = .center
    label.numberOfLines = 0
    label.font = .regular14()
    label.textColor = .white
    label.translatesAutoresizingMaskIntoConstraints = false
    blurView.contentView.addSubview(label)
    blurView.isUserInteractionEnabled = false

    NSLayoutConstraint.activate([
        label.leadingAnchor.constraint(equalTo: blurView.contentView.leadingAnchor, constant: 8),
        label.trailingAnchor.constraint(equalTo: blurView.contentView.trailingAnchor, constant: -8),
        label.topAnchor.constraint(equalTo: blurView.contentView.topAnchor, constant: 8),
        label.bottomAnchor.constraint(equalTo: blurView.contentView.bottomAnchor, constant: -8)
    ])
  }
  
  deinit {
    timer?.invalidate()
  }

  @objc func show() {
    show(in: UIApplication.shared.keyWindow, alignment: .bottom)
  }

  @objc func show(withAlignment alignment: Alignment, pinToSafeArea: Bool = true) {
    show(in: UIApplication.shared.keyWindow, alignment: alignment, pinToSafeArea: pinToSafeArea)
  }

  @objc func show(in view: UIView?, alignment: Alignment, pinToSafeArea: Bool = true) {
    guard let view = view else { return }
    blurView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(blurView)

    let leadingConstraint = blurView.leadingAnchor.constraint(greaterThanOrEqualTo: view.leadingAnchor, constant: 16)
    let trailingConstraint = blurView.trailingAnchor.constraint(lessThanOrEqualTo: view.trailingAnchor, constant: -16)
    
    NSLayoutConstraint.activate([
      leadingConstraint,
      trailingConstraint
    ])
    
    let topConstraint: NSLayoutConstraint
    if alignment == .bottom {
      topConstraint = blurView.bottomAnchor.constraint(equalTo: pinToSafeArea ? view.safeAreaLayoutGuide.bottomAnchor : view.bottomAnchor, constant: -63)
    } else {
      topConstraint = blurView.topAnchor.constraint(equalTo: pinToSafeArea ? view.safeAreaLayoutGuide.topAnchor : view.topAnchor, constant: 50)
    }
    
    NSLayoutConstraint.activate([
      topConstraint,
      blurView.centerXAnchor.constraint(equalTo: pinToSafeArea ? view.safeAreaLayoutGuide.centerXAnchor : view.centerXAnchor)
    ])

    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.blurView.alpha = 1
    }

    timer = Timer.scheduledTimer(timeInterval: 3,
                                 target: self,
                                 selector: #selector(hide),
                                 userInfo: nil,
                                 repeats: false)
  }
  
  @objc func hide() {
    timer?.invalidate()
    if self.blurView.superview != nil {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     animations: { self.blurView.alpha = 0 }) { [self] _ in
        self.blurView.removeFromSuperview()
        Self.toasts.removeAll(where: { $0 === self }) }
    }
  }
}
