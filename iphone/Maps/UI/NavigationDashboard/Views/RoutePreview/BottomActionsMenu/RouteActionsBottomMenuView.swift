final class RouteActionsBottomMenuView: UIView {
  private enum Constants {
    static let height: CGFloat = 44
    static let topInset: CGFloat = 16
    static let horizontalInset: CGFloat = 16
    static let bottomInsetWithoutSafeArea: CGFloat = 8
    fileprivate static let animationDuration: TimeInterval = kDefaultAnimationDuration / 2
    static let spacing: CGFloat = 12
  }

  private let stackView = UIStackView()
  private var bottomConstraint: NSLayoutConstraint?

  init() {
    super.init(frame: .zero)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /// Prevent touches from being passed to the touch transparent view
  override func touchesBegan(_: Set<UITouch>, with _: UIEvent?) {}

  override func safeAreaInsetsDidChange() {
    super.safeAreaInsetsDidChange()
    bottomConstraint?.constant = -Self.bottomInset(for: safeAreaInsets)
  }

  private func setupView() {
    setStyle(.background)
    stackView.axis = .horizontal
    stackView.spacing = Constants.spacing
    stackView.distribution = .fillProportionally
    stackView.translatesAutoresizingMaskIntoConstraints = false
    setHidden(true, animated: false)
  }

  private func layout() {
    addSubview(stackView)
    let bottomConstraint = stackView.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor,
                                                             constant: -Self.bottomInset(for: safeAreaInsets))
    self.bottomConstraint = bottomConstraint
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: Constants.horizontalInset),
      stackView.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -Constants.horizontalInset),
      stackView.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: Constants.topInset),
      bottomConstraint,
      stackView.heightAnchor.constraint(equalToConstant: Constants.height),
    ])
  }

  func addActionView(_ view: UIView) {
    stackView.addArrangedSubview(view)
  }

  func setShadowVisible(_ visible: Bool) {
    guard !isHidden else { return }
    UIView.animate(withDuration: Constants.animationDuration) {
      self.layer.shadowOpacity = visible ? 0.1 : 0
    }
  }

  static func reservedContentHeight(for safeAreaInsets: UIEdgeInsets) -> CGFloat {
    Constants.height + Constants.topInset + bottomInset(for: safeAreaInsets)
  }

  private static func bottomInset(for safeAreaInsets: UIEdgeInsets) -> CGFloat {
    safeAreaInsets.bottom.isZero ? Constants.bottomInsetWithoutSafeArea : .zero
  }
}
