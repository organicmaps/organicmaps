@objc
final class Toast: NSObject {
  @objc
  enum Alignment: Int {
    case bottom
    case top
  }

  private enum Constants {
    static let presentationDuration: TimeInterval = 3
    static let animationDuration: TimeInterval = kDefaultAnimationDuration
    static let bottomOffset: CGFloat = 63
    static let topOffset: CGFloat = 50
    static let horizontalOffset: CGFloat = 16
    static let labelOffsets = UIEdgeInsets(top: 10, left: 14, bottom: -10, right: -14)
    static let maxWidth: CGFloat = 400
  }

  private static var toasts: [Toast] = []
  private var blurView: UIVisualEffectView

  private init(_ text: String) {
    blurView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
    blurView.setStyle(.toastBackground)
    blurView.isUserInteractionEnabled = false
    blurView.alpha = 0
    blurView.translatesAutoresizingMaskIntoConstraints = false

    let label = UILabel()
    label.text = text
    label.setStyle(.toastLabel)
    label.numberOfLines = 0
    label.translatesAutoresizingMaskIntoConstraints = false
    label.setContentHuggingPriority(.defaultLow, for: .horizontal)
    blurView.contentView.addSubview(label)

    NSLayoutConstraint.activate([
      label.leadingAnchor.constraint(equalTo: blurView.contentView.leadingAnchor, constant: Constants.labelOffsets.left),
      label.trailingAnchor.constraint(equalTo: blurView.contentView.trailingAnchor, constant: Constants.labelOffsets.right),
      label.topAnchor.constraint(equalTo: blurView.contentView.topAnchor, constant: Constants.labelOffsets.top),
      label.bottomAnchor.constraint(equalTo: blurView.contentView.bottomAnchor, constant: Constants.labelOffsets.bottom)
    ])
  }

  // MARK: - Public methods

  @objc
  static func show(withText text: String) {
    show(withText: text, alignment: .bottom)
  }

  @objc
  static func show(withText text: String, alignment: Alignment) {
    show(withText: text, alignment: alignment, pinToSafeArea: true)
  }

  @objc
  static func show(withText text: String, alignment: Alignment, pinToSafeArea: Bool) {
    let toast = Toast(text)
    toasts.append(toast)
    toast.show(withAlignment: alignment, pinToSafeArea: pinToSafeArea)
  }

  @objc
  static func hideAll() {
    toasts.forEach { $0.hide() }
  }

  // MARK: - Private methods

  private func show(withAlignment alignment: Alignment, pinToSafeArea: Bool) {
    Self.hideAll()
    guard let view = UIApplication.shared.keyWindow else { return }
    view.addSubview(blurView)

    let leadingConstraint = blurView.leadingAnchor.constraint(greaterThanOrEqualTo: view.leadingAnchor, constant: Constants.horizontalOffset)
    let trailingConstraint = blurView.trailingAnchor.constraint(lessThanOrEqualTo: view.trailingAnchor, constant: -Constants.horizontalOffset)
    let maxWidthConstraint = blurView.widthAnchor.constraint(equalToConstant: Constants.maxWidth).withPriority(.defaultLow)

    let verticalConstraint: NSLayoutConstraint
    switch alignment {
    case .bottom:
      verticalConstraint = blurView.bottomAnchor.constraint(equalTo: pinToSafeArea ? view.safeAreaLayoutGuide.bottomAnchor : view.bottomAnchor,
                                                       constant: -Constants.bottomOffset)
    case .top:
      verticalConstraint = blurView.topAnchor.constraint(equalTo: pinToSafeArea ? view.safeAreaLayoutGuide.topAnchor : view.topAnchor,
                                                    constant: Constants.topOffset)
    }

    NSLayoutConstraint.activate([
      leadingConstraint,
      trailingConstraint,
      maxWidthConstraint,
      verticalConstraint,
      blurView.centerXAnchor.constraint(equalTo: pinToSafeArea ? view.safeAreaLayoutGuide.centerXAnchor : view.centerXAnchor)
    ])

    UIView.animate(withDuration: Constants.animationDuration, animations: {
      self.blurView.alpha = 1
    } , completion: { _ in
      DispatchQueue.main.asyncAfter(deadline: .now() + Constants.presentationDuration) {
        self.hide()
      }
    })
  }

  private func hide() {
    if self.blurView.superview != nil {
      UIView.animate(withDuration: Constants.animationDuration,
                     animations: { self.blurView.alpha = 0 }) { [self] _ in
        self.blurView.removeFromSuperview()
        Self.toasts.removeAll(where: { $0 === self }) }
    }
  }
}
