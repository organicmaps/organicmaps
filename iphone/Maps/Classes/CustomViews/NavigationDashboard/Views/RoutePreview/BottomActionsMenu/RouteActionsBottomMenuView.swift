final class RouteActionsBottomMenuView: UIView {
  enum Constants {
    static let height: CGFloat = 44
    static let insets = UIEdgeInsets(top: 16, left: 16, bottom: MapsAppDelegate.theApp().window.safeAreaInsets.bottom.isZero ? 8 : 0, right: 16)
    fileprivate static let animationDuration: TimeInterval = kDefaultAnimationDuration / 2
    static let spacing: CGFloat = 12
  }

  private let stackView = UIStackView()

  init() {
    super.init(frame: .zero)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // Prevent touches from being passed to the touch transparent view
  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {}

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
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: Constants.insets.left),
      stackView.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -Constants.insets.right),
      stackView.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: Constants.insets.top),
      stackView.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -Constants.insets.bottom),
      stackView.heightAnchor.constraint(equalToConstant: Constants.height)
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
}
