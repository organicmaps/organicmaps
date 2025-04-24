final class StartRouteButton: UIView {

  enum State {
    case enabled
    case loading
    case disabled
    case hidden
  }

  private enum Constants {
    static let buttonHeight: CGFloat = 44
    static let buttonInsets = UIEdgeInsets(top: 16, left: 16, bottom: 16, right: 16)
    static let buttonTitle = L("Start")
    static let animationDuration: TimeInterval = kDefaultAnimationDuration / 2
  }

  private let button = UIButton(type: .system)
  private let activityIndicator: UIActivityIndicatorView = {
    if #available(iOS 13.0, *) {
      let activity = UIActivityIndicatorView(style: .medium)
      activity.color = .white
      return activity
    } else {
      return UIActivityIndicatorView(style: .white)
    }
  }()
  private var onTapAction: (() -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)
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

    button.setTitle(Constants.buttonTitle, for: .normal)
    button.setStyle(.flatNormalButtonBig)
    button.addTarget(self, action: #selector(buttonTapped), for: .touchUpInside)
    button.translatesAutoresizingMaskIntoConstraints = false

    activityIndicator.hidesWhenStopped = true
    activityIndicator.translatesAutoresizingMaskIntoConstraints = false
  }

  private func layout() {
    addSubview(button)
    addSubview(activityIndicator)

    NSLayoutConstraint.activate([
      button.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: Constants.buttonInsets.left),
      button.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -Constants.buttonInsets.right),
      button.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: Constants.buttonInsets.top),
      button.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -Constants.buttonInsets.bottom),
      button.heightAnchor.constraint(equalToConstant: Constants.buttonHeight),

      activityIndicator.centerXAnchor.constraint(equalTo: button.centerXAnchor),
      activityIndicator.centerYAnchor.constraint(equalTo: button.centerYAnchor)
    ])
  }

  func setOnTapAction(_ action: @escaping () -> Void) {
    onTapAction = action
  }

  @objc private func buttonTapped() {
    onTapAction?()
  }

  func setState(_ state: State) {
    UIView.transition(with: self,
                      duration: Constants.animationDuration,
                      options: .transitionCrossDissolve,
                      animations: { [weak self] in
      guard let self = self else { return }
      switch state {
      case .enabled:
        self.button.setTitle(Constants.buttonTitle, for: .normal)
        self.activityIndicator.stopAnimating()
        self.button.isEnabled = true
        self.alpha = 1.0
      case .loading:
        self.button.setTitle(nil, for: .normal)
        self.activityIndicator.startAnimating()
        self.button.isEnabled = false
        self.alpha = 1.0
      case .disabled:
        self.button.setTitle(Constants.buttonTitle, for: .normal)
        self.activityIndicator.stopAnimating()
        self.button.isEnabled = false
        self.alpha = 1.0
      case .hidden:
        self.alpha = 0.0
      }
    })
  }

  func setHidden(_ hidden: Bool) {
    UIView.animate(withDuration: Constants.animationDuration) {
      self.alpha = hidden ? 0 : 1
    } completion: { _ in
      self.isHidden = hidden
    }
  }
}
