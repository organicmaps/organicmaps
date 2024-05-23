@objc(MWMToast)
class Toast: NSObject {
  @objc(MWMToastAlignment)
  enum Alignment: Int {
    case bottom
    case top
  }

  private static let duration: TimeInterval = 3.0
  private static var toasts: [Toast] = []

  private let blurView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
  private var timer: Timer?

  var contentView: UIView { blurView.contentView }
  var onHideCompletion: (() -> Void)?

  override init() {
    super.init()
    setupBlurView()
    Self.toasts.append(self)
  }

  private func setupBlurView() {
    blurView.layer.setCorner(radius: 8)
    blurView.clipsToBounds = true
    blurView.alpha = 0
  }

  deinit {
    timer?.invalidate()
  }
}

// MARK: - Public methods
extension Toast {
  @objc static func hideAll() {
    toasts.forEach { $0.hide() }
  }

  @objc func show() {
    Self.hideAll()
    show(in: UIApplication.shared.keyWindow, alignment: .bottom, duration: Toast.duration)
  }

  @objc func show(withAlignment alignment: Alignment, pinToSafeArea: Bool = true, duration: TimeInterval = Toast.duration) {
    show(in: UIApplication.shared.keyWindow, alignment: alignment, pinToSafeArea: pinToSafeArea, duration: duration)
  }

  @objc func show(in view: UIView?, alignment: Alignment, pinToSafeArea: Bool = true, duration: TimeInterval = Toast.duration) {
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

    timer = Timer.scheduledTimer(timeInterval: duration,
                                 target: self,
                                 selector: #selector(hide),
                                 userInfo: nil,
                                 repeats: false)
  }
  
  @objc func hide() {
    timer?.invalidate()
    if self.blurView.superview != nil {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     animations: { self.blurView.alpha = 0 },
                     completion: { [self] _ in
        self.onHideCompletion?()
        self.blurView.removeFromSuperview()
        Self.toasts.removeAll(where: { $0 === self })
      })
    }
  }
}

extension Toast {
  @objc static func toast(withText text: String) -> Toast {
    toast(withText: text, onHideCompletion: nil)
  }

  @objc static func toast(withText text: String, onHideCompletion: (() -> Void)? = nil) -> Toast {
    DefaultToast(text, onHideCompletion: onHideCompletion)
  }

  @objc static func undoToast(withText text: String, undoAction: @escaping () -> Void, onHideCompletion: (() -> Void)?) -> Toast {
    UndoToast(text, undoAction: undoAction, onHideCompletion: onHideCompletion)
  }

  @objc static func undoToast(deletedObject: String, undoAction: @escaping () -> Void, onHideCompletion: (() -> Void)? = nil) -> Toast {
    // TODO: localize text
    undoToast(withText: "The \(deletedObject) was deleted", undoAction: undoAction, onHideCompletion: onHideCompletion)
  }
}

private final class DefaultToast: Toast {

  private let messageLabel = UILabel()

  fileprivate convenience init(_ text: String, onHideCompletion: (() -> Void)?) {
    self.init()
    self.onHideCompletion = onHideCompletion
    setupMessageLabel(text)
    layoutViews()
  }

  private func setupMessageLabel(_ text: String) {
    messageLabel.text = text
    messageLabel.textAlignment = .center
    messageLabel.numberOfLines = 0
    messageLabel.font = .regular14()
    messageLabel.textColor = .white
    messageLabel.isUserInteractionEnabled = false
  }

  private func layoutViews() {
    contentView.addSubview(messageLabel)
    
    messageLabel.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      messageLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 8),
      messageLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -8),
      messageLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
      messageLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -8)
    ])
  }
}

private final class UndoToast: Toast {

  private let messageLabel = UILabel()
  private let undoButton = UIButton()
  private var undoAction: (() -> Void)?

  fileprivate convenience init(_ text: String, undoAction: @escaping () -> Void, onHideCompletion: (() -> Void)?) {
    self.init()
    self.undoAction = undoAction
    self.onHideCompletion = onHideCompletion
    setupMessageLabel(text)
    setupUndoButton()
    layoutViews()
  }

  private func setupMessageLabel(_ text: String) {
    messageLabel.text = text
    messageLabel.textAlignment = .center
    messageLabel.numberOfLines = 0
    messageLabel.font = .regular14()
    messageLabel.textColor = .white
    messageLabel.isUserInteractionEnabled = false
  }

  private func setupUndoButton() {
    undoButton.setTitle(L("undo"), for: .normal)
    undoButton.setTitleColor(.white, for: .normal)
    undoButton.setTitleColor(.lightGray, for: .highlighted)
    undoButton.titleLabel?.font = .bold17()
    undoButton.backgroundColor = .clear
    undoButton.addTarget(self, action: #selector(undoButtonDidTap), for: .touchUpInside)
  }

  @objc private func undoButtonDidTap() {
    undoAction?()
    hide()
  }

  private func layoutViews() {
    contentView.addSubview(messageLabel)
    contentView.addSubview(undoButton)

    messageLabel.translatesAutoresizingMaskIntoConstraints = false
    undoButton.translatesAutoresizingMaskIntoConstraints = false
    messageLabel.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    NSLayoutConstraint.activate([
      contentView.heightAnchor.constraint(greaterThanOrEqualToConstant: 40),

      messageLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 12),
      messageLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
      messageLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -8),

      undoButton.leadingAnchor.constraint(equalTo: messageLabel.trailingAnchor, constant: 20),
      undoButton.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -25),
      undoButton.centerYAnchor.constraint(equalTo: messageLabel.centerYAnchor),
      undoButton.heightAnchor.constraint(equalToConstant: 30),
    ])
  }
}
