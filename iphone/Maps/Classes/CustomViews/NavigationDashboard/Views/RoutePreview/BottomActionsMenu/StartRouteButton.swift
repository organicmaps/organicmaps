final class StartRouteButton: UIView {

  enum State {
    case enabled
    case loading
    case disabled
  }

  private enum Constants {
    static let buttonTitle = L("p2p_start")
    static let animationDuration: TimeInterval = kDefaultAnimationDuration / 2
  }

  private let button = UIButton(type: .system)
  private var state: State = .enabled
  private let activityIndicator: UIActivityIndicatorView = {
    if #available(iOS 13.0, *) {
      let activity = UIActivityIndicatorView(style: .medium)
      activity.color = .white
      return activity
    } else {
      return UIActivityIndicatorView(style: .white)
    }
  }()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    button.setTitle(Constants.buttonTitle, for: .normal)
    button.setStyle(.flatNormalButtonBig)
    button.titleLabel?.numberOfLines = 1
    button.titleLabel?.minimumScaleFactor = 0.5
    button.titleLabel?.adjustsFontSizeToFitWidth = true
    button.titleLabel?.allowsDefaultTighteningForTruncation = true
    button.translatesAutoresizingMaskIntoConstraints = false

    activityIndicator.hidesWhenStopped = true
    activityIndicator.translatesAutoresizingMaskIntoConstraints = false
  }

  private func layout() {
    addSubview(button)
    addSubview(activityIndicator)

    button.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    NSLayoutConstraint.activate([
      button.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor),
      button.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor),
      button.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor),
      button.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor),

      activityIndicator.centerXAnchor.constraint(equalTo: button.centerXAnchor),
      activityIndicator.centerYAnchor.constraint(equalTo: button.centerYAnchor)
    ])
  }

  func addTarget(_ target: Any?, action: Selector, for controlEvents: UIControl.Event) {
    button.addTarget(target, action: action, for: controlEvents)
  }

  func setState(_ state: State) {
    guard self.state != state else { return }
    self.state = state
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
      case .loading:
        self.button.setTitle(nil, for: .normal)
        self.activityIndicator.startAnimating()
        self.button.isEnabled = false
      case .disabled:
        self.button.setTitle(Constants.buttonTitle, for: .normal)
        self.activityIndicator.stopAnimating()
        self.button.isEnabled = false
      }
    })
  }
}
